#include "app/command_line.h"
#include <string_view>
namespace ai3
{
CommandLine parse_command_line(int argc, char** argv)
{
    CommandLine result;
    for (int index = 1; index < argc; ++index)
    {
        const std::string_view argument(argv[index]);
        if (argument == "--smoke-test")
            result.options.frame_limit = 3;
        else
        {
            result.error = "unknown argument: " + std::string(argument);
            return result;
        }
    }
    return result;
}
} // namespace ai3
