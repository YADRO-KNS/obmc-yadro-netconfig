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
        bool cli_mode = false;

        if (cmd && !strcmp(cmd, "--cli"))
        {
            cli_mode = true;
            ++args;
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
                if (!cli_mode)
                {
                    printf("OpenBMC network configuration tool\n");
                    printf("Copyright (C) 2020-2021 YADRO\n");
                    printf("Version " VERSION "\n\n");
                }
                printf("Usage: %s COMMAND [OPTION...]\n", app);
                printf("       %s help COMMAND\n\n", app);
                printf("COMMANDS:\n");
            }
            help(cli_mode, app, args);
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
