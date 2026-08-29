#pragma once
#include <string>
namespace ai3
{
struct ApplicationOptions
{
    int frame_limit = 0;
};
struct CommandLine
{
    ApplicationOptions options;
    std::string error;
};
CommandLine parse_command_line(int argc, char** argv);
} // namespace ai3
