#include <doctest/doctest.h>

#include "localization/localization.h"
#include "localization/resource_locator.h"

#include <filesystem>
#include <fstream>

namespace
{
class TemporaryLocales
{
    public:
    TemporaryLocales()
        : path_(std::filesystem::temp_directory_path() /
                ("ai3-localization-test-" + std::to_string(++sequence_)))
    {
        std::filesystem::create_directories(path_);
    }
    ~TemporaryLocales() { std::filesystem::remove_all(path_); }
    void write(const std::string& name, const std::string& contents) const
    {
        std::ofstream(path_ / name, std::ios::binary) << contents;
    }
    const std::filesystem::path& path() const { return path_; }

    private:
    static int sequence_;
    std::filesystem::path path_;
};
int TemporaryLocales::sequence_ = 0;

const char* english = R"({
  "locale":"en-US", "name":"English", "font_profile":"latin",
  "strings":{
    "greeting":"Hello", "fallback":"English fallback", "utf8":"café",
    "welcome":"Hello, {name}!", "escaped":"Use {{name}}", "unclosed":"Bad {name",
    "empty":"Bad {}", "unknown":"Bad {missing}", "stray":"Bad } brace",
    "supplementary":"\uD83D\uDE00"
  }
})";
} // namespace

TEST_CASE("locale files load UTF-8 strings and switch at runtime")
{
    TemporaryLocales files;
    files.write("en-US.json", english);
    files.write("es-ES.json", R"({
      "locale":"es-ES", "name":"Español", "font_profile":"latin",
      "strings":{"greeting":"Hola", "utf8":"acción"}
    })");

    ai3::Localization localization(files.path());
    CHECK(localization.active_locale() == "en-US");
    CHECK(localization.text("utf8") == "café");
    REQUIRE(localization.set_locale("es-ES"));
    CHECK(localization.text("greeting") == "Hola");
    CHECK(localization.text("utf8") == "acción");
    CHECK(localization.text("fallback") == "English fallback");
}

TEST_CASE("named interpolation never exposes translation strings as printf formats")
{
    TemporaryLocales files;
    files.write("en-US.json", english);
    ai3::Localization localization(files.path());

    CHECK(localization.format("welcome", {{"name", "AI3"}}) == "Hello, AI3!");
    CHECK(localization.format("escaped", {}) == "Use {name}");
    CHECK(localization.format("unclosed", {{"name", "AI3"}}) == "[format error: unclosed]");
    CHECK(localization.format("empty", {}) == "[format error: empty]");
    CHECK(localization.format("unknown", {{"name", "AI3"}}) == "[format error: unknown]");
    CHECK(localization.format("stray", {}) == "[format error: stray]");
}

TEST_CASE("JSON Unicode escapes combine valid surrogate pairs")
{
    TemporaryLocales files;
    files.write("en-US.json", english);
    ai3::Localization localization(files.path());
    CHECK(localization.text("supplementary") == "\xF0\x9F\x98\x80");
}

TEST_CASE("unknown locales and keys fail visibly without changing locale")
{
    TemporaryLocales files;
    files.write("en-US.json", english);
    ai3::Localization localization(files.path());
    std::string error;
    CHECK_FALSE(localization.set_locale("not-real", &error));
    CHECK_FALSE(error.empty());
    CHECK(localization.active_locale() == "en-US");
    CHECK(localization.text("absent") == "[missing: absent]");
}

TEST_CASE("malformed and incomplete locale resources are rejected")
{
    TemporaryLocales malformed;
    malformed.write("en-US.json", "{ not json }");
    CHECK_THROWS_AS(ai3::Localization(malformed.path()), std::runtime_error);

    TemporaryLocales no_fallback;
    no_fallback.write(
        "es-ES.json",
        R"({"locale":"es-ES","name":"Español","font_profile":"latin","strings":{"a":"b"}})");
    CHECK_THROWS_WITH(ai3::Localization(no_fallback.path()),
                      "required fallback locale en-US is missing");
}

TEST_CASE("JSON Unicode escapes reject lone surrogates")
{
    for (const char* escaped : {"\\uD83D", "\\uDE00", "\\uD83D\\u0041"})
    {
        TemporaryLocales files;
        files.write(
            "en-US.json",
            std::string(
                R"({"locale":"en-US","name":"English","font_profile":"latin","strings":{"bad":")") +
                escaped + R"("}})");
        CHECK_THROWS_AS(ai3::Localization(files.path()), std::runtime_error);
    }
}

TEST_CASE("duplicate locale IDs are rejected")
{
    TemporaryLocales files;
    files.write("one.json", english);
    files.write("two.json", english);
    CHECK_THROWS_WITH(ai3::Localization(files.path()), "duplicate locale ID: en-US");
}

TEST_CASE("resource paths prefer assets beside the executable")
{
    const auto paths = ai3::resource_search_paths("/opt/ai3/bin/ai3", "/work/AI3/assets");
    REQUIRE(paths.size() == 2);
    CHECK(paths[0] == std::filesystem::path("/opt/ai3/bin/assets"));
    CHECK(paths[1] == std::filesystem::path("/work/AI3/assets"));
}
