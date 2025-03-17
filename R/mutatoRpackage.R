# --- New version of mutate_file() that only generates mutants from a file ---
mutate_file <- function(sample_file) {
  # Create a directory to store mutated versions (if not already existing)
  dir.create("./mutations", showWarnings = FALSE)
  
  options(keep.source = TRUE)
  # Parse the sample file while preserving source information
  exprs <- parse(file = sample_file, keep.source = TRUE)
  
  # Call the C++ function to generate mutated expressions
  mutated_expressions <- .Call("C_mutate_file", exprs)
  
  if (!is.list(mutated_expressions)) {
    stop("Error: Expected a list of mutated expressions from C_mutate_file.")
  }
  
  mutated_file_paths <- list()
  
  mutated_file_name <- basename(sample_file)

  # --- Generate mutants based on parsed expressions ---
  for (i in seq_along(mutated_expressions)) {
    # Convert each mutated expression back into R code
    code_lines <- sapply(mutated_expressions[[i]], function(expr) {
      paste(deparse(expr), collapse = "\n")
    })
    mutated_code <- paste(code_lines, collapse = "\n")
    output_file <- sprintf("./mutations/%s_%03d.R", mutated_file_name, i)
    writeLines(mutated_code, output_file)
    mutated_file_paths[[i]] <- output_file
  }
  
  # --- Additional mutation: string-level deletion ---
  original_lines <- readLines(sample_file)
  num_lines <- length(original_lines)
  num_deletion_mutants <- min(5, num_lines)
  
  for (j in 1:num_deletion_mutants) {
    lines_mutated <- original_lines
    # Delete a single random line
    n_delete <- 1
    # add check for non-empty
    delete_indices <- sort(sample(seq_len(num_lines), n_delete, replace = FALSE))
    mutated_lines <- lines_mutated[-delete_indices]
    mutated_code <- paste(mutated_lines, collapse = "\n")
    
    mutant_index <- length(mutated_file_paths) + 1
    output_file <- sprintf("./mutations/%s_%03d.R", mutated_file_name, j)
    writeLines(mutated_code, output_file)
    mutated_file_paths[[mutant_index]] <- output_file
  }
  
  return(mutated_file_paths)
}

# --- Helper function to run all tests in tests/testthat/ of a package ---
run_package_test <- function(pkg_dir) {
  old_dir <- getwd()
  on.exit(setwd(old_dir))
  setwd(pkg_dir)
  
  test_result <- tryCatch({
    results <- testthat::test_dir("tests/testthat"
    , reporter = "silent"
    )
    # If any tests failed, the mutant is considered killed (i.e., test_result = FALSE)
    testthat::failed(results) == 0
  }, error = function(e) {
    # print(e)
    FALSE  # On error, consider the mutant as killed.
  })
  
  return(test_result)
}

# --- mutate_package() uses the new mutate_file() to generate mutants for every R file
# and then runs the test suite for each mutated package copy ---
mutate_package <- function(pkg_path) {
  # Identify R source files in the package's R/ directory.
  r_files <- list.files(file.path(pkg_path, "R"), full.names = TRUE, pattern = "\\.R$")
  package_mutants <- list()
  test_results <- list()
  
  # Loop over each R file in the package
  for (file in r_files) {
    cat("Mutating file:", file, "\n")
    # Generate mutant versions for the current file (without running tests)
    mutants <- mutate_file(file)
    
    # For every mutant, create a complete package copy with the mutant file replacing the original
    for (mutant_file in mutants) {
      # Create a temporary directory to hold the mutated package
      tmp_pkg <- tempfile(pattern = "pkg_mutation_")
      dir.create(tmp_pkg)
      
      # Copy the entire package into the temporary directory.
      # The package copy will be placed in tmp_pkg/<package_name>
      file.copy(pkg_path, tmp_pkg, recursive = TRUE)
      
      # Construct the path to the file in the copied package.
      # If pkg_path is "myPackage", then the copy is in tmp_pkg/myPackage/
      target_file <- file.path(tmp_pkg, basename(pkg_path), "R", basename(file))
      file.copy(mutant_file, target_file, overwrite = TRUE)
      
      # Run tests on the mutant package.
      # Change into the package copy directory.
      pkg_copy_dir <- file.path(tmp_pkg, basename(pkg_path))
      test_result <- run_package_test(pkg_copy_dir)
      status <- if (test_result) "SURVIVED" else "KILLED"
      cat(sprintf("Mutant for %s from %s: %s\n", basename(file), basename(mutant_file), status))
      
      # Record the mutant package location and test result
      mutant_id <- paste(basename(file), basename(mutant_file), sep = "_")
      package_mutants[[mutant_id]] <- pkg_copy_dir
      test_results[[mutant_id]] <- test_result
    }
  }
  
  # Summarize test results:
  total_mutants <- length(test_results)
  survived <- sum(unlist(test_results))
  killed <- total_mutants - survived
  mutation_score <- (killed / total_mutants) * 100
  
  cat("\nMutation Testing Summary:\n")
  cat(sprintf("  Total mutants:    %d\n", total_mutants))
  cat(sprintf("  Killed:           %d\n", killed))
  cat(sprintf("  Survived:         %d\n", survived))
  cat(sprintf("  Mutation Score:   %.2f%%\n", mutation_score))
  
  return(list(package_mutants = package_mutants, test_results = test_results))
}

# --- Example usage ---
# Suppose your package is located in the directory "myPackage"
# This will process each file under myPackage/R/,
# generate mutants, create package copies, run tests, and summarize results.
# mutants_info <- mutate_package("myPackage")
