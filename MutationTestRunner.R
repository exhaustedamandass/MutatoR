# Load the parser library
library(parser)

# Parse the R code into an AST
parsed_code <- parser::parse_r(file = "test_data_cleaning.R")

# Pass the parsed_code to C++ for mutation processing
library(Rcpp)
sourceCpp("mutate_ast.cpp")  # C++ file path with the mutation function

# Generate mutated ASTs
mutated_asts <- generate_mutated_asts(parsed_code)

# Convert and evaluate each mutated AST back to R code
for (mutated_ast in mutated_asts) {
  mutated_code <- deparse(mutated_ast)
  cat("Testing mutation:\n", mutated_code, "\n")
  eval(parse(text = mutated_code))
}

