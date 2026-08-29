#pragma once
#include "app/command_line.h"
namespace ai3
{
class Application
{
    public:
    explicit Application(ApplicationOptions options);
    int run();

    private:
    ApplicationOptions options_;
};
} // namespace ai3
