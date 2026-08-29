#include "localization/localization.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace ai3
{
namespace
{
class JsonReader
{
    public:
    explicit JsonReader(std::string input) : input_(std::move(input)) {}

    Localization::LocaleData read_locale()
    {
        Localization::LocaleData result;
        expect('{');
        while (!consume('}'))
        {
            const std::string key = read_string();
            expect(':');
            if (key == "locale")
                result.info.id = read_string();
            else if (key == "name")
                result.info.name = read_string();
            else if (key == "font_profile")
                result.info.font_profile = read_string();
            else if (key == "strings")
                result.strings = read_string_map();
            else
                skip_value();
            if (!consume('}'))
                expect(',');
            else
            {
                finish();
                return result;
            }
        }
        finish();
        return result;
    }

    private:
    void whitespace()
    {
        while (position_ < input_.size() &&
               std::isspace(static_cast<unsigned char>(input_[position_])))
            ++position_;
    }
    bool consume(char character)
    {
        whitespace();
        if (position_ < input_.size() && input_[position_] == character)
        {
            ++position_;
            return true;
        }
        return false;
    }
    void expect(char character)
    {
        if (!consume(character))
            fail("expected JSON punctuation");
    }
    [[noreturn]] void fail(const char* message) const
    {
        throw std::runtime_error(std::string(message) + " at byte " + std::to_string(position_));
    }
    static void append_utf8(std::string& output, unsigned codepoint)
    {
        if (codepoint <= 0x7f)
            output.push_back(static_cast<char>(codepoint));
        else if (codepoint <= 0x7ff)
        {
            output.push_back(static_cast<char>(0xc0 | (codepoint >> 6)));
            output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
        }
        else if (codepoint <= 0xffff)
        {
            output.push_back(static_cast<char>(0xe0 | (codepoint >> 12)));
            output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
        }
        else
        {
            output.push_back(static_cast<char>(0xf0 | (codepoint >> 18)));
            output.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
        }
    }
    unsigned read_hex4()
    {
        unsigned value = 0;
        for (int digit = 0; digit < 4; ++digit)
        {
            if (position_ >= input_.size())
                fail("incomplete Unicode escape");
            const char character = input_[position_++];
            value *= 16;
            if (character >= '0' && character <= '9')
                value += static_cast<unsigned>(character - '0');
            else if (character >= 'a' && character <= 'f')
                value += static_cast<unsigned>(character - 'a' + 10);
            else if (character >= 'A' && character <= 'F')
                value += static_cast<unsigned>(character - 'A' + 10);
            else
                fail("invalid Unicode escape");
        }
        return value;
    }
    std::string read_string()
    {
        whitespace();
        if (position_ >= input_.size() || input_[position_++] != '"')
            fail("expected JSON string");
        std::string result;
        while (position_ < input_.size())
        {
            const unsigned char character = static_cast<unsigned char>(input_[position_++]);
            if (character == '"')
                return result;
            if (character < 0x20)
                fail("control character in JSON string");
            if (character != '\\')
            {
                result.push_back(static_cast<char>(character));
                continue;
            }
            if (position_ >= input_.size())
                fail("incomplete JSON escape");
            const char escaped = input_[position_++];
            switch (escaped)
            {
            case '"':
            case '\\':
            case '/':
                result.push_back(escaped);
                break;
            case 'b':
                result.push_back('\b');
                break;
            case 'f':
                result.push_back('\f');
                break;
            case 'n':
                result.push_back('\n');
                break;
            case 'r':
                result.push_back('\r');
                break;
            case 't':
                result.push_back('\t');
                break;
            case 'u':
            {
                unsigned codepoint = read_hex4();
                if (codepoint >= 0xd800 && codepoint <= 0xdbff)
                {
                    if (position_ + 2 > input_.size() || input_[position_] != '\\' ||
                        input_[position_ + 1] != 'u')
                        fail("high surrogate without low surrogate");
                    position_ += 2;
                    const unsigned low = read_hex4();
                    if (low < 0xdc00 || low > 0xdfff)
                        fail("high surrogate without low surrogate");
                    codepoint = 0x10000 + ((codepoint - 0xd800) << 10) + (low - 0xdc00);
                }
                else if (codepoint >= 0xdc00 && codepoint <= 0xdfff)
                    fail("lone low surrogate");
                append_utf8(result, codepoint);
                break;
            }
            default:
                fail("invalid JSON escape");
            }
        }
        fail("unterminated JSON string");
    }
    std::unordered_map<std::string, std::string> read_string_map()
    {
        std::unordered_map<std::string, std::string> result;
        expect('{');
        if (consume('}'))
            return result;
        for (;;)
        {
            std::string key = read_string();
            expect(':');
            const auto inserted = result.emplace(std::move(key), read_string());
            if (!inserted.second)
                fail("duplicate localization key");
            if (consume('}'))
                return result;
            expect(',');
        }
    }
    void skip_value()
    {
        whitespace();
        if (position_ >= input_.size())
            fail("missing JSON value");
        if (input_[position_] == '"')
        {
            (void)read_string();
            return;
        }
        fail("unsupported JSON value");
    }
    void finish()
    {
        whitespace();
        if (position_ != input_.size())
            fail("trailing JSON data");
    }

    std::string input_;
    std::size_t position_ = 0;
};
} // namespace

Localization::Localization(std::filesystem::path locale_directory)
{
    if (!std::filesystem::is_directory(locale_directory))
        throw std::runtime_error("locale directory not found: " + locale_directory.string());
    for (const auto& entry : std::filesystem::directory_iterator(locale_directory))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".json")
            locales_.push_back(load_file(entry.path()));
    }
    std::sort(locales_.begin(), locales_.end(), [](const LocaleData& left, const LocaleData& right)
              { return left.info.id < right.info.id; });
    for (std::size_t index = 1; index < locales_.size(); ++index)
    {
        if (locales_[index - 1].info.id == locales_[index].info.id)
            throw std::runtime_error("duplicate locale ID: " + locales_[index].info.id);
    }
    for (const LocaleData& locale : locales_)
        locale_info_.push_back(locale.info);
    fallback_ = find_locale("en-US");
    if (fallback_ == nullptr)
        throw std::runtime_error("required fallback locale en-US is missing");
    active_ = fallback_;
}

Localization::LocaleData Localization::load_file(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error("unable to open locale file: " + path.string());
    LocaleData data =
        JsonReader(std::string(std::istreambuf_iterator<char>(stream), {})).read_locale();
    if (data.info.id.empty() || data.info.name.empty() || data.info.font_profile.empty() ||
        data.strings.empty())
        throw std::runtime_error("locale file is missing required fields: " + path.string());
    return data;
}

const Localization::LocaleData* Localization::find_locale(std::string_view id) const
{
    for (const LocaleData& locale : locales_)
        if (locale.info.id == id)
            return &locale;
    return nullptr;
}

bool Localization::set_locale(std::string_view locale, std::string* error)
{
    const LocaleData* selected = find_locale(locale);
    if (selected == nullptr)
    {
        if (error != nullptr)
            *error = "locale is not available: " + std::string(locale);
        return false;
    }
    active_ = selected;
    return true;
}

const std::string& Localization::text(std::string_view key) const
{
    const auto active_value = active_->strings.find(std::string(key));
    if (active_value != active_->strings.end())
        return active_value->second;
    const auto fallback_value = fallback_->strings.find(std::string(key));
    if (fallback_value != fallback_->strings.end())
        return fallback_value->second;
    const std::string key_string(key);
    return missing_keys_.try_emplace(key_string, "[missing: " + key_string + "]").first->second;
}

std::string Localization::format(std::string_view key, FormatArguments arguments) const
{
    const std::string& pattern = text(key);
    std::string result;
    result.reserve(pattern.size());
    for (std::size_t position = 0; position < pattern.size();)
    {
        if (pattern[position] == '{')
        {
            if (position + 1 < pattern.size() && pattern[position + 1] == '{')
            {
                result.push_back('{');
                position += 2;
                continue;
            }
            const std::size_t close = pattern.find('}', position + 1);
            if (close == std::string::npos || close == position + 1)
                return "[format error: " + std::string(key) + "]";
            const std::string_view placeholder(pattern.data() + position + 1, close - position - 1);
            const auto argument =
                std::find_if(arguments.begin(), arguments.end(),
                             [&](const auto& candidate) { return candidate.first == placeholder; });
            if (argument == arguments.end())
                return "[format error: " + std::string(key) + "]";
            result.append(argument->second);
            position = close + 1;
            continue;
        }
        if (pattern[position] == '}')
        {
            if (position + 1 < pattern.size() && pattern[position + 1] == '}')
            {
                result.push_back('}');
                position += 2;
                continue;
            }
            return "[format error: " + std::string(key) + "]";
        }
        result.push_back(pattern[position++]);
    }
    return result;
}

const std::string& Localization::active_locale() const { return active_->info.id; }
const std::string& Localization::font_profile() const { return active_->info.font_profile; }
const std::vector<LocaleInfo>& Localization::available_locales() const { return locale_info_; }
} // namespace ai3
