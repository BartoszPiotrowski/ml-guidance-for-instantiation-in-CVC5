/******************************************************************************
 * Top contributors (to current version):
 *   Mathias Preiner, Aina Niemetz
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2021 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Option template for option modules.
 *
 * For each <module>_options.toml configuration file, mkoptions.py
 * expands this template and generates a <module>_options.cpp file.
 */

#include "options/options_holder.h"
#include "base/check.h"

// clang-format off
namespace cvc5 {

${accs}$


namespace options {

${defs}$

${modes}$

}  // namespace options
}  // namespace cvc5
// clang-format on