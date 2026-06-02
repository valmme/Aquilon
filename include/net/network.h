#ifndef AQUILON_NET_NETWORK_H
#define AQUILON_NET_NETWORK_H

#include <cstddef>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

#define AQUILON_NETWORK_MAX_PACKET_SIZE 8192

typedef enum NetworkMode {
    NETWORK_MODE_NONE = 0,
    NETWORK_MODE_SERVER,
    NETWORK_MODE_CLIENT
} NetworkMode;

typedef enum NetworkEventType {
    NETWORK_EVENT_NONE = 0,
    NETWORK_EVENT_CONNECTED,
    NETWORK_EVENT_DISCONNECTED,
    NETWORK_EVENT_PACKET,
    NETWORK_EVENT_ERROR
} NetworkEventType;

typedef struct NetworkConfig {
    int listen_backlog;
} NetworkConfig;

typedef struct NetworkEvent {
    NetworkEventType type;
    int peer_id;
    std::size_t payload_size;
    char text[192];
    std::uint8_t payload[AQUILON_NETWORK_MAX_PACKET_SIZE];
} NetworkEvent;

typedef struct NetworkContext NetworkContext;

NetworkConfig Network_DefaultConfig(void);
NetworkContext* Network_Create(const NetworkConfig* config);
void Network_Destroy(NetworkContext* context);

bool Network_Host(NetworkContext* context, const char* bind_host, std::uint16_t port);
bool Network_Connect(NetworkContext* context, const char* host, std::uint16_t port);
void Network_Disconnect(NetworkContext* context);

void Network_Update(NetworkContext* context);
bool Network_PollEvent(NetworkContext* context, NetworkEvent* out_event);

bool Network_Send(NetworkContext* context, int peer_id, const void* data, std::size_t size);
bool Network_Broadcast(NetworkContext* context, const void* data, std::size_t size);

NetworkMode Network_GetMode(const NetworkContext* context);
bool Network_IsActive(const NetworkContext* context);
const char* Network_GetLastError(const NetworkContext* context);

#ifdef __cplusplus
}
#endif

#endif // AQUILON_NET_NETWORK_H
