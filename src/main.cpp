#include "app/application.h"
#include "app/command_line.h"

#include <cstdio>

int main(int argc, char** argv)
{
    const ai3::CommandLine command_line = ai3::parse_command_line(argc, argv);
    if (!command_line.error.empty())
    {
        std::fprintf(stderr, "ai3: %s\nUsage: ai3 [--smoke-test]\n", command_line.error.c_str());
        return 2;
    }
    return ai3::Application(command_line.options).run();
}
