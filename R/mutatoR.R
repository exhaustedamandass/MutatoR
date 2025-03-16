# mutate_and_test.R
library(testthat)
#' @useDynLib MutatoR, .registration = TRUE
NULL

# 2. Define a function to mutate a single file and run tests on each mutant

#' Mutate a File and Run Tests
#'
#' This function parses an R script file, applies mutations to it 
#' using a C++ backend, and tests each mutated version.
#'
#' @param sample_file Path to the sample R script file to mutate.
#' @param test_file Path to the R script file containing the tests to run on
#'   the mutated versions.
#' @return A summary of the mutation testing results.
#' @export
mutate_file <- function(sample_file, test_file) {
  # Create a 'mutations' directory to store mutated versions
  dir.create("./mutations", showWarnings = FALSE)

  options(keep.source = TRUE)
  # Parse the sample file with source information preserved
  exprs <- parse(file = sample_file, keep.source = TRUE)
  str(exprs)

  # Call the C++ function to generate mutated expressions
  mutated_expressions <- .Call("C_mutate_file", exprs)

  if (!is.list(mutated_expressions)) {
    stop("Error: Expected a list of mutated expressions from C_mutate_file.")
  }

  # Loop over each mutated expression (mutation on parsed expressions)
  results <- list()
  for (i in seq_along(mutated_expressions)) {
    # Deparse each individual expression and collapse into valid R code
    code_lines <- sapply(mutated_expressions[[i]], function(expr) {
      paste(deparse(expr), collapse = "\n")
    })
    mutated_code <- paste(code_lines, collapse = "\n")
    
    # Write the valid R code to a file
    output_file <- sprintf("./mutations_reduced/mutated_%03d.R", i)
    writeLines(mutated_code, output_file)
    
    # Retrieve the "mutation_info" attribute for each expression
    mutation_info <- attr(mutated_expressions[[i]], "mutation_info")
    if (is.null(mutation_info) || mutation_info == "") mutation_info <- "<no info>"
    
    test_result <- run_mutant_test(output_file, test_file)
    status_str  <- if (test_result) "SURVIVED" else "KILLED"
    
    cat(sprintf("Mutant %03d => %s\n", i, status_str))
    cat(sprintf("Mutation info: %s\n", mutation_info))
    cat(sprintf("   Result: %s\n\n", status_str))
    
    results[[i]] <- test_result
  }
  
  # --- New mutation strategy: string-level deletion ---
  # Read the original sample file as text (a vector of lines)
  original_lines <- readLines(sample_file)
  num_lines <- length(original_lines)
  
  # Define how many string-level mutants to generate.
  # Ensure that the number does not exceed the total number of lines.
  num_deletion_mutants <- min(5, num_lines)
  
  for (j in 1:num_deletion_mutants) {
    # Copy the original lines to mutate
    lines_mutated <- original_lines
    
    # Randomly decide how many lines to delete (at least one, at most half the total)
    n_delete <- 1
    # Choose random indices to delete
    delete_indices <- sort(sample(seq_len(num_lines), n_delete, replace = FALSE))
    
    # Remove the selected lines
    mutated_lines <- lines_mutated[-delete_indices]
    mutated_code <- paste(mutated_lines, collapse = "\n")
    
    # Continue numbering mutants after those generated above
    mutant_index <- length(results) + 1
    output_file <- sprintf("./mutations_reduced/mutated_%03d.R", mutant_index)
    writeLines(mutated_code, output_file)
    
    # Create a simple description of the mutation for reporting
    mutation_info <- sprintf("String-level deletion: deleted line(s) %s", paste(delete_indices, collapse = ", "))
    
    test_result <- run_mutant_test(output_file, test_file)
    status_str  <- if (test_result) "SURVIVED" else "KILLED"
    
    cat(sprintf("Mutant %03d => %s\n", mutant_index, status_str))
    cat(sprintf("Mutation info: %s\n", mutation_info))
    cat(sprintf("   Result: %s\n\n", status_str))
    
    results[[mutant_index]] <- test_result
  }
  
  # --- Final summary ---
  total_mutants <- length(results)
  killed <- sum(!unlist(results))
  survived <- sum(unlist(results))
  mutation_score <- (killed / total_mutants) * 100
  
  cat("\nMutations saved in './mutations' directory.\n")
  cat("Summary:\n")
  cat(sprintf("  Total mutants:    %d\n", total_mutants))
  cat(sprintf("  Killed:           %d\n", killed))
  cat(sprintf("  Survived:         %d\n", survived))
  cat(sprintf("  Mutation Score:   %.2f%%\n", mutation_score))
}


# if all tests pass, returns TRUE -> mutant SURVIVED
# if any of tests fail, returns FALSE -> mutant KILLED
run_mutant_test <- function(mutant_file, test_file) {
  # 3a. Create an isolated environment so the mutated code does not
  #     contaminate our current workspace.
  #     We'll load the mutated file in this environment.
  mutant_env <- new.env(parent = globalenv())
  
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
  
  # 3c. Run the unit test using testthat without printing logs to the console.
  #     The test file is expected to reference objects from 'mutant_env'.
  test_result <- tryCatch(
    expr = {
      test_env <- new.env(parent = mutant_env)
      # Run tests with a silent reporter to suppress logs.
      results <- testthat::test_file(test_file, env = test_env
      # ,reporter = "silent"
      )
      # If any tests fail, the mutant is considered killed.
      if (testthat::failed(results) > 0) {
        FALSE
      } else {
        TRUE
      }
    },
    error = function(e) {
      FALSE  # If an error occurs while running the tests, treat as a killed mutant.
    }
  )
  
  # 3d. Return TRUE if the mutant SURVIVED (all tests passed),
  #     FALSE if the mutant was KILLED (at least one test failed).
  return(test_result)
}
