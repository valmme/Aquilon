#ifndef AQUILON_LOCALIZATION_H
#define AQUILON_LOCALIZATION_H

#include <cstdint>
#include <string>
#include <string_view>

struct LocalizationInfo {
    std::string internal_name;
    std::string display_name;
    std::string numeric_id;
    std::string code;
};

std::uint64_t hash(std::string_view text);
std::uint64_t hash(const char* text);

bool LoadLocalization(const std::string& path);
const LocalizationInfo& GetLocalizationInfo();

std::string FindString(std::uint64_t key_hash, std::uint64_t section_hash = hash(""));
std::string Localize(std::string_view text, std::string_view section = {});

#endif // AQUILON_LOCALIZATION_H
