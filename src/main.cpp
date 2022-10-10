// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020-2021 YADRO

#include "netconfig.hpp"
#include "version.hpp"

#include <sdbusplus/exception.hpp>

#include <cstring>

bool isHelp(const char* str)
{
    return str && (!strcmp(str, "help") || !strcmp(str, "--help") ||
                   !strcmp(str, "-h"));
}

void printAbout()
{
    printf("OpenBMC network configuration tool\n");
    printf("Copyright (C) 2020-2021 YADRO\n");
    printf("Version " VERSION "\n\n");
}

void printNetconfigHelp()
{
    printAbout();

    printf("Usage: netconfig COMMAND SUBCOMMAND [OPTION...]\n");
    printf("       netconfig COMMAND help\n");
    printf("       netconfig COMMAND help SUBCOMMAND\n\n");
    printf("COMMANDS:\n");
    printf("  ifconfig\tNetwork configuration commands\n");
    printf("  syslog\tRemote syslog server commands\n");
}

CLIMode setMode(const char* cmd)
{
    CLIMode mode = CLIMode::normalMode;
    if (cmd && !strncmp(cmd, "--cli", 5))
    {
        mode = CLIMode::cliMode;
        if (!strcmp(cmd, "--cli-hide-cmd"))
        {
            mode = CLIMode::cliModeNoCommand;
        }
    }

    return mode;
}

bool parseNetconfigCmd(const char* cmd)
{
    if (cmd && (!strcmp(cmd, ifcfg) || !strcmp(cmd, sslg)))
    {
        return true;
    }
    else
    {
        if (!isHelp(cmd))
        {
            printf("%s is not a valid command\n\n", cmd);
        }
        printNetconfigHelp();
        return false;
    }
}

/** @brief Application entry point. */
int main(int argc, char* argv[])
{
    Arguments args(argc, argv);
    const char* app = args.asText();

    try
    {
        const char* cmd = args.peek();
        CLIMode mode = setMode(cmd);
        std::string app_str{app};

        if (!strcmp(app, netCnfg))
        {
            if (parseNetconfigCmd(cmd))
            {
                app_str += (" " + std::string{cmd});
            }
            else
            {
                return EXIT_SUCCESS;
            }
        }

        ++args;
        cmd = args.peek();

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
                    printAbout();
                }
                printf("Usage: %s COMMAND [OPTION...]\n", app_str.c_str());
                printf("       %s help COMMAND\n\n", app_str.c_str());
                printf("COMMANDS:\n");
            }
            help(mode, app_str.c_str(), args);
        }
        else
        {
            execute(app_str.c_str(), args);
        }
    }
    catch (sdbusplus::exception::SdBusError& ex)
    {
        std::string what = ex.what();
        if (what.find("UnreachableGW") != std::string::npos)
            fprintf(stderr, "Unreachable gateway specified\n");
        else if (what.find("NotAllowed") != std::string::npos)
            fprintf(stderr, "The operation is not allowed because no static "
                            "addresses found\n");
        else
            fprintf(stderr, "%s\n", ex.what());

        return EXIT_FAILURE;
    }
    catch (std::exception& ex)
    {
        fprintf(stderr, "%s\n", ex.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
