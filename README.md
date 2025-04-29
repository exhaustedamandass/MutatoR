# MutatoR

## Overview

MutatoR is an automated mutation testing tool for the R language. It applies mutation testing principles to help developers improve test suite quality by introducing small, systematic changes (mutations) to source code and verifying if tests can detect these changes.

## Features

- **Comprehensive Mutation Testing**: Applies various mutation operators to R source code
- **AST-Based Mutation**: Uses Abstract Syntax Tree analysis for intelligent code mutations
- **Parallel Test Execution**: Runs tests in parallel for improved performance
- **Equivalent Mutant Detection**: Uses AI to identify mutants that are functionally equivalent to the original code
- **Detailed Reporting**: Provides mutation scores and analysis of test suite effectiveness

## Package Structure

```
MutatoR/
├── R/                      # R source code
│   ├── mutatoRpackage.R    # Core functionality
│   └── sample/             # Sample R code
├── src/                    # C++ source code for mutation operators
├── tests/                  # Package tests
├── mutations/              # Generated mutations (created at runtime)
├── .openai_config.R.template # Template for OpenAI API configuration
└── DESCRIPTION             # Package metadata
```

## Installation

```r
# Install dependencies
install.packages(c("devtools", "testthat", "httr", "jsonlite", "future", "furrr"))

# Install from GitHub (if available)
# devtools::install_github("username/MutatoR")

# Or install from local source
# In R console
setwd("path/to/MutatoR")
devtools::install()
```

## Quick Start Guide

```r
library(MutatoR)

# Mutate a single file
mutants <- mutate_file("path/to/your/file.R")

# Mutate an entire package and run tests
result <- mutate_package("path/to/your/package")
```

## Configuration

### Equivalent Mutant Detection

To use the equivalent mutant detection feature with OpenAI:

1. Set up your OpenAI API key in one of these ways:
   - Set the environment variable `OPENAI_API_KEY`
   - Create a file at `~/.openai_config.R` based on the provided template `.openai_config.R.template`

2. Optionally, specify the model to use:
   - Set the environment variable `OPENAI_MODEL` (default is "gpt-4")
   - Define it in the config file

## Mutation Operators

MutatoR implements a wide range of mutation operators to thoroughly test your code:

### Arithmetic and Logical Operator Replacements

| Operator | Description | Example |
|----------|-------------|---------|
| cxx_add_to_sub | Replaces `+` with `-` | `a + b` → `a - b` |
| cxx_sub_to_add | Replaces `-` with `+` | `a - b` → `a + b` |
| cxx_mul_to_div | Replaces `*` with `/` | `a * b` → `a / b` |
| cxx_div_to_mul | Replaces `/` with `*` | `a / b` → `a * b` |
| cxx_eq_to_ne | Replaces `==` with `!=` | `a == b` → `a != b` |
| cxx_ne_to_eq | Replaces `!=` with `==` | `a != b` → `a == b` |
| cxx_gt_to_ge | Replaces `>` with `>=` | `a > b` → `a >= b` |
| cxx_gt_to_le | Replaces `>` with `<=` | `a > b` → `a <= b` |
| cxx_lt_to_le | Replaces `<` with `<=` | `a < b` → `a <= b` |
| cxx_lt_to_ge | Replaces `<` with `>=` | `a < b` → `a >= b` |
| cxx_ge_to_gt | Replaces `>=` with `>` | `a >= b` → `a > b` |
| cxx_ge_to_lt | Replaces `>=` with `<` | `a >= b` → `a < b` |
| cxx_le_to_lt | Replaces `<=` with `<` | `a <= b` → `a < b` |
| cxx_le_to_gt | Replaces `<=` with `>` | `a <= b` → `a > b` |
| cxx_and_to_or | Replaces `&` with `\|` | `a & b` → `a \| b` |
| cxx_or_to_and | Replaces `\|` with `&` | `a \| b` → `a & b` |
| cxx_logical_and_to_or | Replaces `&&` with `\|\|` | `a && b` → `a \|\| b` |
| cxx_logical_or_to_and | Replaces `\|\|` with `&&` | `a \|\| b` → `a && b` |

### Unary Operator Mutations

| Operator | Description | Example |
|----------|-------------|---------|
| cxx_minus_to_noop | Removes unary minus | `-x` → `x` |
| cxx_remove_negation | Removes logical negation | `!x` → `x` |

### Assignment and Value Mutations

| Operator | Description | Example |
|----------|-------------|---------|
| cxx_assign_const | Replaces assignment with constant | `a = b` → `a = 42` |
| cxx_replace_scalar_call | Replaces function call with constant | `f(x)` → `42` |
| scalar_value_mutator | Replaces constants | `0` → `42`, non-zero → `0` |
| negate_mutator | Negates conditionals | `x` → `!x`, `!x` → `x` |

## Dependencies

MutatoR depends on:
- **R Packages**:
  - **devtools**: For package development utilities
  - **testthat**: For test execution
  - **future** and **furrr**: For parallel execution
  - **httr** and **jsonlite**: For OpenAI API integration
  
- **C++11**: For efficient mutation operator implementation

## Building from Source

The package includes C++ components that need to be compiled:

```bash
# Set compiler flags
export PKG_CPPFLAGS="-std=c++11 -I/path/to/R/include -I/path/to/cpp11/include"

# Compile
R CMD SHLIB mutateR.cpp

# Or use make
make
```
