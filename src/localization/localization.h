#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ai3
{
struct LocaleInfo
{
    std::string id;
    std::string name;
    std::string font_profile;
};

class Localization
{
    public:
    struct LocaleData
    {
        LocaleInfo info;
        std::unordered_map<std::string, std::string> strings;
    };

    explicit Localization(std::filesystem::path locale_directory);

    bool set_locale(std::string_view locale, std::string* error = nullptr);
    const std::string& text(std::string_view key) const;
    const std::string& active_locale() const;
    const std::string& font_profile() const;
    const std::vector<LocaleInfo>& available_locales() const;

    private:
    static LocaleData load_file(const std::filesystem::path& path);
    const LocaleData* find_locale(std::string_view id) const;

    std::vector<LocaleData> locales_;
    std::vector<LocaleInfo> locale_info_;
    const LocaleData* fallback_ = nullptr;
    const LocaleData* active_ = nullptr;
    mutable std::unordered_map<std::string, std::string> missing_keys_;
};
} // namespace ai3
