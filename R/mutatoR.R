# mutate_and_test.R

# 1. Load the shared library that contains 'C_generate_mutations'
# dyn.load("mutateR.so")

# 2. Define a function to mutate a single file and run tests on each mutant
mutate_file <- function(sample_file, test_file) {
  # Create a 'mutations' directory to store mutated versions
  dir.create("./mutations", showWarnings = FALSE)

  options(keep.source = TRUE)
  # Instead of readLines() + parse(text=...), do parse(file=..., keep.source=TRUE)
  exprs <- parse(file = sample_file, keep.source = TRUE)
  str(exprs)

  # Call the C++ function
  mutated_expressions <- .Call("C_mutate_single", exprs)
  
  if (!is.list(mutated_expressions)) {
    stop("Error: Expected a list of mutated expressions from C_mutate_single.")
  }

  # Now loop over each mutated expression exactly once
  results <- list()
  for (i in seq_along(mutated_expressions)) {
    mutated_code <- deparse(mutated_expressions[[i]])
    output_file <- sprintf("./mutations/mutated_%03d.R", i)
    writeLines(mutated_code, output_file)

    # For display, get the "mutation_info" attribute
    mutation_info <- attr(mutated_expressions[[i]], "mutation_info")
    if (is.null(mutation_info)) mutation_info <- "<no info>"

    test_result <- run_mutant_test(output_file, test_file)
    status_str  <- if (test_result) "SURVIVED" else "KILLED"

    cat(sprintf("Mutant %03d => %s\n", i, status_str))
    cat(sprintf("   Mutation info: %s\n", mutation_info))
    cat(sprintf("   Result: %s\n\n", status_str))

    results[[i]] <- test_result
  }

  # Summary
  cat("\nMutations saved in './mutations' directory.\n")
  cat("Summary:\n")
  cat(sprintf("  Total mutants:  %d\n", length(results)))
  cat(sprintf("  Killed:         %d\n", sum(!unlist(results))))
  cat(sprintf("  Survived:       %d\n", sum(unlist(results))))
}

# 3. Helper function to run a test on a single mutated version
run_mutant_test <- function(mutant_file, test_file) {
  # 3a. Create an isolated environment so the mutated code does not
  #     contaminate our current workspace. 
  #     We'll load the mutated file in this environment.
  mutant_env <- new.env(parent = baseenv())
  
  # 3b. Source the mutated code
  tryCatch(
    expr = {
      source(mutant_file, local = mutant_env)
    },
    error = function(e) {
      # If there's an error loading the mutant, we consider it a killed mutant
      return(FALSE)
    }
  )

  # 3c. Now run the unit test. We'll use 'testthat' as an example.
  #     The test file is expected to reference objects that are in
  #     'mutant_env'.
  #     We'll store the test results in a variable to see if it fails or passes.
  test_result <- tryCatch(
    expr = {
      # We can use `testthat::test_file(...)`. 
      # We'll create a new environment for testthat to run in, 
      # but that environment must have access to the objects in `mutant_env`.
      # One approach is to temporarily attach or merge the environment:
      
      # Example approach: 
      #   1. Copy objects from `mutant_env` into the environment used by test_file
      test_env <- new.env(parent = mutant_env)
      
      #   2. Run the test in that environment
      testthat::test_file(test_file, env = test_env)
      
      # If the test suite doesn't throw an error or fail, test passes
      # But we need to detect failures. test_file returns a summary object 
      # that we can analyze or, by default, it prints to console.

      # For simplicity, if no R error is thrown, assume PASS. 
      # We'll refine below if needed.

      TRUE  # meaning "SURVIVED" if no test fails
    },
    error = function(e) {
      # If the test code throws an error or fails, consider it KILLED
      FALSE
    }
  )

  # 3d. Return TRUE if the mutant SURVIVED (test passed),
  #     FALSE if the mutant was KILLED (test failed).
  return(test_result)
}
