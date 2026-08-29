#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "app/command_line.h"

TEST_CASE("the default command line runs interactively")
{
    char program[] = "ai3";
    char* arguments[] = {program};
    const ai3::CommandLine result = ai3::parse_command_line(1, arguments);
    CHECK(result.error.empty());
    CHECK(result.options.frame_limit == 0);
}

TEST_CASE("smoke mode has a bounded frame count")
{
    char program[] = "ai3";
    char option[] = "--smoke-test";
    char* arguments[] = {program, option};
    const ai3::CommandLine result = ai3::parse_command_line(2, arguments);
    CHECK(result.error.empty());
    CHECK(result.options.frame_limit > 0);
}

TEST_CASE("unknown options are rejected")
{
    char program[] = "ai3";
    char option[] = "--not-an-option";
    char* arguments[] = {program, option};
    const ai3::CommandLine result = ai3::parse_command_line(2, arguments);
    CHECK(result.options.frame_limit == 0);
    CHECK(result.error == "unknown argument: --not-an-option");
}
