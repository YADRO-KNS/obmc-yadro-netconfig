// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

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
        if (!cmd || strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 ||
            strcmp(cmd, "-h") == 0)
        {
            if (cmd)
            {
                ++args;
            }
            if (!args.peek())
            {
                printf("OpenBMC network configuration.\n");
                printf("Copyright (c) 2020 YADRO.\n");
                printf("Version " VERSION ".\n");
                printf("Usage: %s COMMAND [OPTION...]\n", app);
            }
            help(args);
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
