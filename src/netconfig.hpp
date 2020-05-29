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

/**
 * @brief Print usage help.
 *
 * @param[in] args command line arguments
 */
void help(Arguments& args);
