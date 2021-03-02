// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020-2021 YADRO

#include "netconfig.hpp"
#include "version.hpp"

#include <cstring>

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

        if (!cmd || strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 ||
            strcmp(cmd, "-h") == 0)
        {
            if (cmd)
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
