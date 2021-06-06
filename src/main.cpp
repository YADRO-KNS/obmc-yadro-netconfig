// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020-2021 YADRO

#include "netconfig.hpp"
#include "version.hpp"

#include <cstring>

bool isHelp(const char* str)
{
    return str && (!strcmp(str, "help") || !strcmp(str, "--help") ||
                   !strcmp(str, "-h"));
}

/** @brief Application entry point. */
int main(int argc, char* argv[])
{
    Arguments args(argc, argv);
    const char* app = args.asText();

    try
    {
        const char* cmd = args.peek();
        CLIMode mode = CLIMode::normalMode;

        if (cmd && !strncmp(cmd, "--cli", 5))
        {
            mode = CLIMode::cliMode;
            ++args;

            if (!strcmp(cmd, "--cli-hide-cmd"))
            {
                mode = CLIMode::cliModeNoCommand;
            }

            cmd = args.peek();
        }

        const char* firstArg = args.peekNext();
        const bool firstArgHelp = isHelp(firstArg); // Save on strcmp calls

        if (!cmd || isHelp(cmd) || firstArgHelp)
        {
            if (cmd && !firstArgHelp)
            {
                ++args;
            }
            if (!args.peek())
            {
                if (mode == CLIMode::normalMode)
                {
                    printf("OpenBMC network configuration tool\n");
                    printf("Copyright (C) 2020-2021 YADRO\n");
                    printf("Version " VERSION "\n\n");
                }
                printf("Usage: %s COMMAND [OPTION...]\n", app);
                printf("       %s help COMMAND\n\n", app);
                printf("COMMANDS:\n");
            }
            help(mode, app, args);
        }
        else
        {
            execute(args);
        }
    }
    catch (std::exception& ex)
    {
        fprintf(stderr, "%s\n", ex.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
