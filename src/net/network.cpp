#include "net/network.h"
#include "logger.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <deque>
#include <new>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
using SocketHandle = SOCKET;
static constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
using SocketHandle = int;
static constexpr SocketHandle kInvalidSocket = -1;
#endif

namespace {
constexpr std::size_t kProtocolHeaderSize = 4;

struct PendingEvent {
    NetworkEvent event;
};

struct PeerConnection {
    int id = 0;
    SocketHandle socket = kInvalidSocket;
    bool connected = false;
    std::string address_text;
    std::vector<std::uint8_t> receive_buffer;
    std::size_t receive_offset = 0;
    std::deque<std::vector<std::uint8_t>> send_queue;
    std::size_t send_offset = 0;
};
}

struct NetworkContext {
    NetworkMode mode = NETWORK_MODE_NONE;
    bool active = false;
    SocketHandle listen_socket = kInvalidSocket;
    int next_peer_id = 1;
    NetworkConfig config{};
    std::vector<PeerConnection> peers;
    std::deque<PendingEvent> events;
    std::string last_error;
};

namespace {

static NetworkEvent MakeEvent(NetworkEventType type, int peer_id, const char* text) {
    NetworkEvent event{};
    event.type = type;
    event.peer_id = peer_id;
    event.payload_size = 0;
    if (text && *text) {
        std::snprintf(event.text, sizeof(event.text), "%s", text);
    } else {
        event.text[0] = '\0';
    }
    return event;
}

static void PushEvent(NetworkContext* context, const NetworkEvent& event) {
    if (!context) {
        return;
    }

    PendingEvent pending{};
    pending.event = event;
    context->events.push_back(pending);
}

static void QueueError(NetworkContext* context, const std::string& text) {
    if (!context) {
        return;
    }

    context->last_error = text;
    Logger::Log("NETWORK", Logger::Level::Error, "%s", text.c_str());
    NetworkEvent event = MakeEvent(NETWORK_EVENT_ERROR, 0, text.c_str());
    PushEvent(context, event);
}

#ifdef _WIN32
static std::string GetSocketErrorText(const char* prefix) {
    const int code = WSAGetLastError();
    char buffer[256];
    std::snprintf(buffer, sizeof(buffer), "%s (WSA %d)", prefix, code);
    return buffer;
}

static bool IsWouldBlock(int error) {
    return error == WSAEWOULDBLOCK;
}

static bool InitSockets(std::string& error) {
    WSADATA data{};
    const int result = WSAStartup(MAKEWORD(2, 2), &data);
    if (result != 0) {
        error = "WSAStartup failed";
        return false;
    }
    return true;
}

static void ShutdownSockets() {
    WSACleanup();
}

static bool SetNonBlocking(SocketHandle socket, std::string& error) {
    u_long mode = 1;
    if (ioctlsocket(socket, FIONBIO, &mode) != 0) {
        error = GetSocketErrorText("Failed to enable non-blocking mode");
        return false;
    }
    return true;
}

static void CloseSocket(SocketHandle socket) {
    if (socket != kInvalidSocket) {
        closesocket(socket);
    }
}

static int SocketCloseResult() {
    return SOCKET_ERROR;
}

using SocketLength = int;
#else
static std::string GetSocketErrorText(const char* prefix) {
    const int code = errno;
    char buffer[256];
    std::snprintf(buffer, sizeof(buffer), "%s (errno %d)", prefix, code);
    return buffer;
}

static bool IsWouldBlock(int error) {
    return error == EWOULDBLOCK || error == EAGAIN;
}

static bool InitSockets(std::string&) {
    return true;
}

static void ShutdownSockets() {
}

static bool SetNonBlocking(SocketHandle socket, std::string& error) {
    const int flags = fcntl(socket, F_GETFL, 0);
    if (flags < 0) {
        error = GetSocketErrorText("Failed to read socket flags");
        return false;
    }

    if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        error = GetSocketErrorText("Failed to enable non-blocking mode");
        return false;
    }
    return true;
}

static void CloseSocket(SocketHandle socket) {
    if (socket != kInvalidSocket) {
        close(socket);
    }
}

static int SocketCloseResult() {
    return -1;
}

using SocketLength = socklen_t;
#endif

static std::string MakeEndpointText(const sockaddr_storage& address) {
    char host[NI_MAXHOST]{};
    char service[NI_MAXSERV]{};
    const int result = getnameinfo(
        reinterpret_cast<const sockaddr*>(&address),
        (SocketLength)((address.ss_family == AF_INET) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6)),
        host,
        sizeof(host),
        service,
        sizeof(service),
        NI_NUMERICHOST | NI_NUMERICSERV);

    if (result != 0) {
        return "<unknown>";
    }

    std::string endpoint = host;
    endpoint += ':';
    endpoint += service;
    return endpoint;
}

static bool ResolveAddress(const char* host, std::uint16_t port, bool passive, sockaddr_storage& out_address, SocketLength& out_length, std::string& error) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if (passive) {
        hints.ai_flags = AI_PASSIVE;
    }

    char service[16];
    std::snprintf(service, sizeof(service), "%u", (unsigned)port);

    addrinfo* result = nullptr;
    const int status = getaddrinfo((host && *host) ? host : nullptr, service, &hints, &result);
    if (status != 0 || !result) {
        error = "Failed to resolve network address";
        return false;
    }

    std::memcpy(&out_address, result->ai_addr, result->ai_addrlen);
    out_length = (SocketLength)result->ai_addrlen;
    freeaddrinfo(result);
    return true;
}

static bool CreateTcpSocket(int family, SocketHandle& out_socket, std::string& error) {
    out_socket = socket(family, SOCK_STREAM, IPPROTO_TCP);
    if (out_socket == kInvalidSocket) {
        error = GetSocketErrorText("Failed to create socket");
        return false;
    }
    return true;
}

static bool SetReuseAddress(SocketHandle socket) {
    int reuse = 1;
    return setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse)) == 0;
}

static void DisconnectPeer(NetworkContext* context, PeerConnection& peer, const char* reason) {
    if (!peer.connected) {
        return;
    }

    CloseSocket(peer.socket);
    peer.socket = kInvalidSocket;
    peer.connected = false;

    if (reason && *reason) {
        Logger::Log("NETWORK", Logger::Level::Info, "Peer %d disconnected: %s", peer.id, reason);
        NetworkEvent event = MakeEvent(NETWORK_EVENT_DISCONNECTED, peer.id, reason);
        PushEvent(context, event);
    } else {
        Logger::Log("NETWORK", Logger::Level::Info, "Peer %d disconnected.", peer.id);
        NetworkEvent event = MakeEvent(NETWORK_EVENT_DISCONNECTED, peer.id, "Disconnected");
        PushEvent(context, event);
    }
}

static PeerConnection* FindPeer(NetworkContext* context, int peer_id) {
    if (!context) {
        return nullptr;
    }

    for (PeerConnection& peer : context->peers) {
        if (peer.id == peer_id && peer.connected) {
            return &peer;
        }
    }
    return nullptr;
}

static void CompactReceiveBuffer(PeerConnection& peer) {
    if (peer.receive_offset == 0) {
        return;
    }

    if (peer.receive_offset >= peer.receive_buffer.size()) {
        peer.receive_buffer.clear();
        peer.receive_offset = 0;
        return;
    }

    if (peer.receive_offset < peer.receive_buffer.size() / 2) {
        return;
    }

    peer.receive_buffer.erase(peer.receive_buffer.begin(), peer.receive_buffer.begin() + (std::ptrdiff_t)peer.receive_offset);
    peer.receive_offset = 0;
}

static bool ReadFromPeer(NetworkContext* context, PeerConnection& peer) {
    std::uint8_t buffer[2048];
    for (;;) {
        const int received = recv(peer.socket, reinterpret_cast<char*>(buffer), (int)sizeof(buffer), 0);
        if (received > 0) {
            peer.receive_buffer.insert(peer.receive_buffer.end(), buffer, buffer + received);
            continue;
        }

        if (received == 0) {
            DisconnectPeer(context, peer, "Remote closed the connection");
            return false;
        }

        const int error = 
#ifdef _WIN32
            WSAGetLastError();
#else
            errno;
#endif

        if (IsWouldBlock(error)) {
            break;
        }

        context->last_error = GetSocketErrorText("Socket receive failed");
        DisconnectPeer(context, peer, context->last_error.c_str());
        return false;
    }

    while (peer.receive_buffer.size() - peer.receive_offset >= kProtocolHeaderSize) {
        const std::uint8_t* base = peer.receive_buffer.data() + peer.receive_offset;
        const std::uint32_t payload_size =
            (std::uint32_t)base[0] |
            ((std::uint32_t)base[1] << 8) |
            ((std::uint32_t)base[2] << 16) |
            ((std::uint32_t)base[3] << 24);

        if (payload_size > AQUILON_NETWORK_MAX_PACKET_SIZE) {
            context->last_error = "Incoming packet exceeded the maximum packet size";
            DisconnectPeer(context, peer, context->last_error.c_str());
            return false;
        }

        if (peer.receive_buffer.size() - peer.receive_offset < kProtocolHeaderSize + payload_size) {
            break;
        }

        NetworkEvent event = MakeEvent(NETWORK_EVENT_PACKET, peer.id, "Packet received");
        event.payload_size = payload_size;
        if (payload_size > 0) {
            std::memcpy(event.payload, base + kProtocolHeaderSize, payload_size);
        }
        Logger::Log("NETWORK", Logger::Level::Debug,
                    "Received packet from peer %d (%u bytes).",
                    peer.id, (unsigned)payload_size);
        PushEvent(context, event);
        peer.receive_offset += kProtocolHeaderSize + payload_size;
        CompactReceiveBuffer(peer);
    }

    return true;
}

static bool FlushPeer(NetworkContext* context, PeerConnection& peer) {
    while (!peer.send_queue.empty()) {
        std::vector<std::uint8_t>& packet = peer.send_queue.front();
        while (peer.send_offset < packet.size()) {
            const int sent = send(
                peer.socket,
                reinterpret_cast<const char*>(packet.data() + peer.send_offset),
                (int)(packet.size() - peer.send_offset),
                0);

            if (sent > 0) {
                peer.send_offset += (std::size_t)sent;
                continue;
            }

            if (sent == SocketCloseResult()) {
                const int error = 
#ifdef _WIN32
                    WSAGetLastError();
#else
                    errno;
#endif

                if (IsWouldBlock(error)) {
                    return true;
                }

                context->last_error = GetSocketErrorText("Socket send failed");
                DisconnectPeer(context, peer, context->last_error.c_str());
                return false;
            }
        }

        peer.send_queue.pop_front();
        peer.send_offset = 0;
    }

    return true;
}

static void PumpListeningSocket(NetworkContext* context) {
    if (!context || context->listen_socket == kInvalidSocket) {
        return;
    }

    for (;;) {
        sockaddr_storage client_address{};
        SocketLength client_length = sizeof(client_address);
        SocketHandle client_socket = accept(
            context->listen_socket,
            reinterpret_cast<sockaddr*>(&client_address),
            &client_length);

        if (client_socket == kInvalidSocket) {
            const int error = 
#ifdef _WIN32
                WSAGetLastError();
#else
                errno;
#endif

            if (IsWouldBlock(error)) {
                break;
            }

            context->last_error = GetSocketErrorText("Failed to accept a connection");
            QueueError(context, context->last_error);
            break;
        }

        std::string socket_error;
        if (!SetNonBlocking(client_socket, socket_error)) {
            CloseSocket(client_socket);
            QueueError(context, socket_error);
            continue;
        }

        PeerConnection peer{};
        peer.id = context->next_peer_id++;
        peer.socket = client_socket;
        peer.connected = true;
        peer.address_text = MakeEndpointText(client_address);
        context->peers.push_back(std::move(peer));

        const PeerConnection& stored = context->peers.back();
        Logger::Log("NETWORK", Logger::Level::Info,
                    "Accepted peer %d from %s.",
                    stored.id, stored.address_text.c_str());
        NetworkEvent event = MakeEvent(NETWORK_EVENT_CONNECTED, stored.id, stored.address_text.c_str());
        PushEvent(context, event);
    }
}

static void PumpPeers(NetworkContext* context) {
    for (PeerConnection& peer : context->peers) {
        if (!peer.connected) {
            continue;
        }

        if (ReadFromPeer(context, peer)) {
            FlushPeer(context, peer);
        }
    }

    context->peers.erase(
        std::remove_if(context->peers.begin(), context->peers.end(), [](const PeerConnection& peer) {
            return !peer.connected;
        }),
        context->peers.end());

    if (context->mode == NETWORK_MODE_CLIENT && context->peers.empty()) {
        context->active = false;
    }
}

static void ResetContext(NetworkContext* context) {
    if (!context) {
        return;
    }

    for (PeerConnection& peer : context->peers) {
        CloseSocket(peer.socket);
        peer.socket = kInvalidSocket;
        peer.connected = false;
    }
    context->peers.clear();

    if (context->listen_socket != kInvalidSocket) {
        CloseSocket(context->listen_socket);
        context->listen_socket = kInvalidSocket;
    }

    context->mode = NETWORK_MODE_NONE;
    context->active = false;
    context->next_peer_id = 1;
}
}

NetworkConfig Network_DefaultConfig(void) {
    NetworkConfig config{};
    config.listen_backlog = 8;
    return config;
}

NetworkContext* Network_Create(const NetworkConfig* config) {
    std::string error;
    if (!InitSockets(error)) {
        Logger::Log("NETWORK", Logger::Level::Fatal, "Socket backend initialization failed: %s", error.c_str());
        return nullptr;
    }

    NetworkContext* context = new (std::nothrow) NetworkContext();
    if (!context) {
        ShutdownSockets();
        return nullptr;
    }
    if (config) {
        context->config = *config;
    } else {
        context->config = Network_DefaultConfig();
    }

    if (context->config.listen_backlog <= 0) {
        context->config.listen_backlog = 8;
    }

    Logger::Log("NETWORK", Logger::Level::Info,
                "Networking module initialized (backlog=%d).",
                context->config.listen_backlog);
    return context;
}

void Network_Destroy(NetworkContext* context) {
    if (!context) {
        return;
    }

    ResetContext(context);
    delete context;
    ShutdownSockets();
    Logger::Log("NETWORK", Logger::Level::Info, "Networking module shut down.");
}

bool Network_Host(NetworkContext* context, const char* bind_host, std::uint16_t port) {
    if (!context) {
        return false;
    }

    Network_Disconnect(context);

    sockaddr_storage address{};
    SocketLength address_length = 0;
    std::string resolve_error;
    if (!ResolveAddress(bind_host, port, true, address, address_length, resolve_error)) {
        context->last_error = resolve_error;
        QueueError(context, context->last_error);
        return false;
    }

    SocketHandle socket_handle = kInvalidSocket;
    if (!CreateTcpSocket(address.ss_family, socket_handle, context->last_error)) {
        QueueError(context, context->last_error);
        return false;
    }

    if (!SetReuseAddress(socket_handle)) {
        context->last_error = GetSocketErrorText("Failed to enable address reuse");
        CloseSocket(socket_handle);
        QueueError(context, context->last_error);
        return false;
    }

    if (bind(socket_handle, reinterpret_cast<const sockaddr*>(&address), address_length) == SocketCloseResult()) {
        context->last_error = GetSocketErrorText("Failed to bind socket");
        CloseSocket(socket_handle);
        QueueError(context, context->last_error);
        return false;
    }

    if (listen(socket_handle, context->config.listen_backlog) == SocketCloseResult()) {
        context->last_error = GetSocketErrorText("Failed to listen on socket");
        CloseSocket(socket_handle);
        QueueError(context, context->last_error);
        return false;
    }

    std::string non_blocking_error;
    if (!SetNonBlocking(socket_handle, non_blocking_error)) {
        CloseSocket(socket_handle);
        QueueError(context, non_blocking_error);
        return false;
    }

    context->listen_socket = socket_handle;
    context->mode = NETWORK_MODE_SERVER;
    context->active = true;
    context->last_error.clear();
    Logger::Log("NETWORK", Logger::Level::Info,
                "Hosting TCP server on %s:%u.",
                bind_host && *bind_host ? bind_host : "0.0.0.0",
                (unsigned)port);
    return true;
}

bool Network_Connect(NetworkContext* context, const char* host, std::uint16_t port) {
    if (!context || !host || !*host) {
        return false;
    }

    Network_Disconnect(context);

    sockaddr_storage address{};
    SocketLength address_length = 0;
    std::string resolve_error;
    if (!ResolveAddress(host, port, false, address, address_length, resolve_error)) {
        context->last_error = resolve_error;
        QueueError(context, context->last_error);
        return false;
    }

    SocketHandle socket_handle = kInvalidSocket;
    if (!CreateTcpSocket(address.ss_family, socket_handle, context->last_error)) {
        QueueError(context, context->last_error);
        return false;
    }

    if (connect(socket_handle, reinterpret_cast<const sockaddr*>(&address), address_length) == SocketCloseResult()) {
        context->last_error = GetSocketErrorText("Failed to connect to remote host");
        CloseSocket(socket_handle);
        QueueError(context, context->last_error);
        return false;
    }

    std::string non_blocking_error;
    if (!SetNonBlocking(socket_handle, non_blocking_error)) {
        CloseSocket(socket_handle);
        QueueError(context, non_blocking_error);
        return false;
    }

    PeerConnection peer{};
    peer.id = 0;
    peer.socket = socket_handle;
    peer.connected = true;
    peer.address_text = MakeEndpointText(address);
    context->peers.push_back(std::move(peer));

    context->mode = NETWORK_MODE_CLIENT;
    context->active = true;
    context->last_error.clear();
    Logger::Log("NETWORK", Logger::Level::Info,
                "Connected to %s.",
                context->peers.back().address_text.c_str());
    PushEvent(context, MakeEvent(NETWORK_EVENT_CONNECTED, 0, context->peers.back().address_text.c_str()));
    return true;
}

void Network_Disconnect(NetworkContext* context) {
    if (!context) {
        return;
    }

    if (context->active) {
        Logger::Log("NETWORK", Logger::Level::Info, "Disconnecting networking context.");
    }
    ResetContext(context);
}

void Network_Update(NetworkContext* context) {
    if (!context || !context->active) {
        return;
    }

    PumpListeningSocket(context);
    PumpPeers(context);
}

bool Network_PollEvent(NetworkContext* context, NetworkEvent* out_event) {
    if (!context || !out_event || context->events.empty()) {
        return false;
    }

    *out_event = context->events.front().event;
    context->events.pop_front();
    return true;
}

bool Network_Send(NetworkContext* context, int peer_id, const void* data, std::size_t size) {
    if (!context || !context->active || (!data && size > 0) || size > AQUILON_NETWORK_MAX_PACKET_SIZE) {
        if (context && size > AQUILON_NETWORK_MAX_PACKET_SIZE) {
            context->last_error = "Outgoing packet exceeded the maximum packet size";
            QueueError(context, context->last_error);
        }
        return false;
    }

    PeerConnection* peer = nullptr;
    if (context->mode == NETWORK_MODE_CLIENT) {
        peer = FindPeer(context, 0);
    } else if (context->mode == NETWORK_MODE_SERVER) {
        peer = FindPeer(context, peer_id);
    }

    if (!peer) {
        context->last_error = "Unknown peer";
        QueueError(context, context->last_error);
        return false;
    }

    std::vector<std::uint8_t> packet;
    packet.resize(kProtocolHeaderSize + size);
    const std::uint32_t payload_size = (std::uint32_t)size;
    packet[0] = (std::uint8_t)(payload_size & 0xFFu);
    packet[1] = (std::uint8_t)((payload_size >> 8) & 0xFFu);
    packet[2] = (std::uint8_t)((payload_size >> 16) & 0xFFu);
    packet[3] = (std::uint8_t)((payload_size >> 24) & 0xFFu);
    if (size > 0) {
        std::memcpy(packet.data() + kProtocolHeaderSize, data, size);
    }
    peer->send_queue.push_back(std::move(packet));
    return true;
}

bool Network_Broadcast(NetworkContext* context, const void* data, std::size_t size) {
    if (!context || context->mode != NETWORK_MODE_SERVER) {
        return false;
    }

    bool all_ok = true;
    for (PeerConnection& peer : context->peers) {
        if (!peer.connected) {
            continue;
        }
        all_ok = Network_Send(context, peer.id, data, size) && all_ok;
    }
    return all_ok;
}

NetworkMode Network_GetMode(const NetworkContext* context) {
    if (!context) {
        return NETWORK_MODE_NONE;
    }
    return context->mode;
}

bool Network_IsActive(const NetworkContext* context) {
    return context && context->active;
}

const char* Network_GetLastError(const NetworkContext* context) {
    if (!context) {
        return "No network context";
    }

    if (context->last_error.empty()) {
        return "";
    }

    return context->last_error.c_str();
}
