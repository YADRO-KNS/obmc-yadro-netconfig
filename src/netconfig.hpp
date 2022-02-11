// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#pragma once

#include "arguments.hpp"

/**
 * @brief Execute the configuration command.
 *
 * @param[in] args command line arguments
 *
 * @throw std::exception in case of errors
 */
void execute(Arguments& args);

enum class CLIMode {
  normalMode, ///< Normal mode, print the banner and the command name in help
  cliMode, ///< CLI mode, do not print banner, print command name in help
  cliModeNoCommand ///< CLI mode, print neither the banner, nor the command name in help
};

/**
 * @brief Print usage help.
 *
 * @param[in] cli_mode CLI mode. Don't prepend arguments info with
 *                     the command name for single-command help.
 * @param[in] app      application name
 * @param[in] args     command line arguments
 */
void help(CLIMode mode, const char *app, Arguments& args);

/** @brief IEEE 802.1Q VLAN ID limits. */
static constexpr uint32_t minVlanId = 2;
static constexpr uint32_t maxVlanId = 4094;
