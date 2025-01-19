# MutatoR
Automated mutation testing tool for R language

# compiling cpp source code 
g++ -g main.cpp -std=c++11 -I/usr/share/R/include -I/home/asanaliamandykov.linux/R/aarch64-unknown-linux-gnu-library/4.3/cpp11/include -L/usr/lib/R/lib -lR

R_HOME=/usr/lib/R ./a.out

# making a package

export PKG_CPPFLAGS="-std=c++11 -I/home/asanaliamandykov.linux/R/aarch64-unknown-linux-gnu-library/4.3/cpp11/include -I/usr/share/R/include"

R CMD SHLIB mutateR.cpp

R

source("mutatoR.R")

mutate_file("sample/sample.R")

# mutation operators

1. **Arithmetic and Logical Operator Replacements:**
    - **cxx_add_to_sub:** Replaces `+` with
    - **cxx_and_to_or:** Replaces `&` with `|`
    - **cxx_div_to_mul:** Replaces `/` with
    - **cxx_eq_to_ne:** Replaces `==` with `!=`
    - **cxx_ge_to_gt:** Replaces `>=` with `>`
    - **cxx_ge_to_lt:** Replaces `>=` with `<`
    - **cxx_gt_to_ge:** Replaces `>` with `>=`
    - **cxx_gt_to_le:** Replaces `>` with `<=`
    - **cxx_le_to_gt:** Replaces `<=` with `>`
    - **cxx_le_to_lt:** Replaces `<=` with `<`
    - **cxx_logical_and_to_or:** Replaces `&&` with `||`
    - **cxx_logical_or_to_and:** Replaces `||` with `&&`
    - **cxx_lt_to_ge:** Replaces `<` with `>=`
    - **cxx_lt_to_le:** Replaces `<` with `<=`
    - **cxx_mul_to_div:** Replaces  with `/`
    - **cxx_ne_to_eq:** Replaces `!=` with `==`
    - **cxx_or_to_and:** Replaces `|` with `&`
    - **cxx_sub_to_add:** Replaces  with `+`
2. **Unary Operator Mutations:**
    - **cxx_minus_to_noop:** Replaces unary minus `x` with `x`
    - **cxx_remove_negation:** Replaces `!a` with `a` (removing logical negation)
3. **Assignment and Value Replacement:**
    - **cxx_assign_const:** Replaces `a = b` with `a = 42`
    - **cxx_replace_scalar_call:** Replaces a function call with `42`
4. **Other Mutation Operators:**
    - **negate_mutator:** Negates conditionals (`!x` to `x` and `x` to `!x`)
    - **scalar_value_mutator:** Replaces zeros with 42 and non-zeros with 0

use makefile

make
