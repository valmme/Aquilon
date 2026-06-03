#include "localization.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <unordered_map>

#include "logger.h"

namespace {
struct LocalizedEntry {
    std::string source;
    std::string translation;
};

using LocalizedMap = std::unordered_map<std::uint64_t, LocalizedEntry>;
using SectionMap = std::unordered_map<std::uint64_t, LocalizedMap>;

struct LocalizationState {
    LocalizationInfo info;
    SectionMap sections;
    bool loaded = false;
};

LocalizationState& State() {
    static LocalizationState state;
    return state;
}

std::string Trim(const std::string& value) {
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });

    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();

    if (begin >= end) {
        return {};
    }

    return std::string(begin, end);
}

std::string ToLower(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

bool StartsWith(const std::string& value, const char* prefix) {
    const std::size_t prefix_length = std::char_traits<char>::length(prefix);
    return value.size() >= prefix_length && value.compare(0, prefix_length, prefix) == 0;
}

std::uint64_t HashBytes(const char* data, std::size_t size) {
    constexpr std::uint64_t kOffset = 1469598103934665603ull;
    constexpr std::uint64_t kPrime = 1099511628211ull;

    std::uint64_t value = kOffset;
    for (std::size_t i = 0; i < size; ++i) {
        value ^= static_cast<unsigned char>(data[i]);
        value *= kPrime;
    }
    return value;
}

void CommitEntry(SectionMap& sections, std::uint64_t section_hash, std::uint64_t key_hash, const std::string& source, const std::string& translation) {
    sections[section_hash][key_hash] = LocalizedEntry{source, translation.empty() ? source : translation};
}
} // namespace

std::uint64_t hash(std::string_view text) {
    return HashBytes(text.data(), text.size());
}

std::uint64_t hash(const char* text) {
    if (!text) {
        return hash(std::string_view{});
    }

    return HashBytes(text, std::char_traits<char>::length(text));
}

bool LoadLocalization(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        Logger::Log("SYSTEM", Logger::Level::Warn,
                    "Localization file '%s' not found; using source strings.",
                    path.c_str());
        return false;
    }

    LocalizationState loaded_state;
    std::string line;
    int line_number = 0;
    bool in_header = true;
    int header_line = 0;
    std::string current_section_name;
    std::uint64_t current_section_hash = hash("");
    std::string pending_source;
    std::uint64_t pending_key_hash = 0;
    bool have_pending = false;

    auto commit_pending = [&]() {
        if (have_pending) {
            CommitEntry(loaded_state.sections, current_section_hash, pending_key_hash, pending_source, pending_source);
            have_pending = false;
            pending_source.clear();
            pending_key_hash = 0;
        }
    };

    while (std::getline(file, line)) {
        ++line_number;

        const std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }

        if (trimmed.front() == '[' && trimmed.back() == ']') {
            commit_pending();
            in_header = false;
            current_section_name = Trim(trimmed.substr(1, trimmed.size() - 2));
            current_section_hash = hash(current_section_name);
            continue;
        }

        if (in_header && header_line == 0 && !StartsWith(trimmed, "==")) {
            loaded_state.info.internal_name = trimmed;
            header_line = 1;
            continue;
        }

        if (in_header && StartsWith(trimmed, "==")) {
            const std::string value = Trim(trimmed.substr(2));
            switch (header_line) {
                case 1: loaded_state.info.display_name = value; break;
                case 2: loaded_state.info.numeric_id = value; break;
                case 3: loaded_state.info.code = ToLower(value); in_header = false; break;
                default: break;
            }
            ++header_line;
            continue;
        }

        if (StartsWith(trimmed, "==")) {
            if (!have_pending) {
                Logger::Log("SYSTEM", Logger::Level::Warn,
                            "Ignoring stray translation line %d in '%s'.",
                            line_number, path.c_str());
                continue;
            }

            const std::string translation = Trim(trimmed.substr(2));
            CommitEntry(loaded_state.sections, current_section_hash, pending_key_hash, pending_source, translation);
            have_pending = false;
            pending_source.clear();
            pending_key_hash = 0;
            continue;
        }

        if (have_pending) {
            CommitEntry(loaded_state.sections, current_section_hash, pending_key_hash, pending_source, pending_source);
        }

        pending_source = trimmed;
        pending_key_hash = hash(trimmed);
        have_pending = true;
    }

    commit_pending();

    loaded_state.loaded = true;
    State() = std::move(loaded_state);

    const LocalizationInfo& info = State().info;
    if (!info.code.empty() || !info.internal_name.empty()) {
        Logger::Log("SYSTEM", Logger::Level::Info,
                    "Loaded localization '%s' (%s).",
                    info.display_name.empty() ? info.internal_name.c_str() : info.display_name.c_str(),
                    info.code.empty() ? "unknown" : info.code.c_str());
    } else {
        Logger::Log("SYSTEM", Logger::Level::Info,
                    "Loaded localization file '%s'.",
                    path.c_str());
    }

    return true;
}

const LocalizationInfo& GetLocalizationInfo() {
    return State().info;
}

std::string FindString(std::uint64_t key_hash, std::uint64_t section_hash) {
    const LocalizationState& state = State();
    const auto find_in_section = [&](std::uint64_t section) -> const LocalizedEntry* {
        const auto section_it = state.sections.find(section);
        if (section_it == state.sections.end()) {
            return nullptr;
        }

        const auto entry_it = section_it->second.find(key_hash);
        if (entry_it == section_it->second.end()) {
            return nullptr;
        }

        return &entry_it->second;
    };

    if (const LocalizedEntry* entry = find_in_section(section_hash)) {
        return entry->translation.empty() ? entry->source : entry->translation;
    }

    if (section_hash != hash("")) {
        if (const LocalizedEntry* entry = find_in_section(hash(""))) {
            return entry->translation.empty() ? entry->source : entry->translation;
        }
    }

    return {};
}

std::string Localize(std::string_view text, std::string_view section) {
    const std::string translated = FindString(hash(text), hash(section));
    if (!translated.empty()) {
        return translated;
    }

    return std::string(text);
}
