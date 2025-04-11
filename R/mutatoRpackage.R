mutate_file_new <- function(sample_file) {
  # Create a directory to store mutated versions (if not already existing)
  dir.create("./mutations", showWarnings = FALSE)
  
  options(keep.source = TRUE)
  # Parse the sample file while preserving source information
  exprs <- parse(file = sample_file, keep.source = TRUE)
  
  # Call the C++ function to generate mutated expressions
  mutated_expressions <- .Call("C_mutate_file", exprs)
  
  mutated_file_paths <- list()
  
  mutated_file_name <- basename(sample_file)

  # Check if mutated_expressions is a valid list
  if (!is.list(mutated_expressions) || length(mutated_expressions) == 0) {
    cat("No mutated expressions generated, proceeding with string-level deletion.\n")
  } else {
    # --- Generate mutants based on parsed expressions ---
    for (i in seq_along(mutated_expressions)) {
      # Convert each mutated expression back into R code
      code_lines <- sapply(mutated_expressions[[i]], function(expr) {
        paste(deparse(expr), collapse = "\n")
      })
      mutated_code <- paste(code_lines, collapse = "\n")
      output_file <- sprintf("./mutations/%s_%03d.R", mutated_file_name, i)
      writeLines(mutated_code, output_file)
      
      # Retrieve and store mutation info
      mutation_info <- attr(mutated_expressions[[i]], "mutation_info")
      print(mutation_info)
      if (is.null(mutation_info) || mutation_info == "") mutation_info <- "<no info>"
      
      mutated_file_paths[[i]] <- list(
        path = output_file,
        mutation_info = mutation_info
      )
    }
  }
  
  # --- Additional mutation: string-level deletion ---
  original_lines <- readLines(sample_file)
  num_lines <- length(original_lines)
  num_deletion_mutants <- min(5, num_lines)
  
  # Filter out empty lines
  non_empty_lines <- which(nzchar(original_lines))
  
  for (j in 1:num_deletion_mutants) {
    lines_mutated <- original_lines
    # Delete a single random non-empty line
    n_delete <- 1
    if (length(non_empty_lines) > 0) {
      delete_indices <- sort(sample(non_empty_lines, n_delete, replace = FALSE))
      mutated_lines <- lines_mutated[-delete_indices]
      mutated_code <- paste(mutated_lines, collapse = "\n")
      
      mutant_index <- length(mutated_file_paths) + 1
      output_file <- sprintf("./mutations/%s_%03d.R", mutated_file_name, j)
      writeLines(mutated_code, output_file)
      
      # Create mutation info for string-level deletion
      mutation_info <- sprintf("String-level deletion: deleted line(s) %s", paste(delete_indices, collapse = ", "))
      
      mutated_file_paths[[mutant_index]] <- list(
        path = output_file,
        mutation_info = mutation_info
      )
    }
  }
  
  return(mutated_file_paths)
}

# --- Helper function to run all tests in tests/testthat/ of a package ---
run_package_test <- function(pkg_dir) {
  old_dir <- getwd()
  on.exit(setwd(old_dir))
  setwd(pkg_dir)
  
  test_result <- tryCatch({
    # First try to load/compile the package
    devtools_result <- tryCatch({
      devtools::load_all(quiet = TRUE)
      TRUE
    }, error = function(e) {
      cat("Compilation error:", conditionMessage(e), "\n")
      NULL  # Return NULL for compilation errors
    })
    
    # Only run tests if compilation succeeded
    if (!is.null(devtools_result)) {
      results <- testthat::test_dir("tests/testthat")
      # If any tests failed, the mutant is considered killed
      testthat::failed(results) == 0
    } else {
      NULL
    }
  }, error = function(e) {
    print(e)
    NULL
  })
  
  return(test_result)
}

mutate_package <- function(pkg_path, cores = parallel::detectCores()) {
  r_files <- list.files(file.path(pkg_path, "R"), full.names = TRUE, pattern = "\\.R$")
  
  all_mutants <- list()
  
  for (file in r_files) {
    cat("Mutating file:", file, "\n")
    mutants <- mutate_file_new(file)
    
    # For every mutant, create a complete package copy with the mutant file replacing the original.
    for (mutant_file in mutants) {
      # Create a temporary directory to hold the mutated package.
      tmp_pkg <- tempfile(pattern = "pkg_mutation_")
      dir.create(tmp_pkg)
      
      # Copy the entire package into the temporary directory.
      file.copy(pkg_path, tmp_pkg, recursive = TRUE)
      
      # Construct the path to the file in the copied package.
      target_file <- file.path(tmp_pkg, basename(pkg_path), "R", basename(file))
      file.copy(mutant_file$path, target_file, overwrite = TRUE)
      
      # Create a unique mutant identifier.
      mutant_id <- paste(basename(file), basename(mutant_file$path), sep = "_")
      
      # Store the package copy directory and mutation info for later lookup.
      all_mutants[[mutant_id]] <- list(
        pkg_copy_dir = file.path(tmp_pkg, basename(pkg_path)),
        mutation_info = mutant_file$mutation_info
      )
    }
  }
  
  package_mutants <- list()
  test_results <- list()
  
  # --- Running tests ---
  if (length(all_mutants) > 1) {
    cat(sprintf("Running tests in parallel using %d cores...\n", cores))
    
    # Setup furrr parallel processing
    future::plan(future::multisession, workers = min(cores, length(all_mutants)))
    
    mutant_ids <- names(all_mutants)
    pkg_dirs <- sapply(all_mutants, function(x) x$pkg_copy_dir)
    
    # Create a named list for future mapping
    pkg_dir_list <- setNames(as.list(pkg_dirs), mutant_ids)
    
    # Run tests in parallel with progress bar
    parallel_results <- furrr::future_map(
      pkg_dir_list, 
      run_package_test,
      .progress = TRUE,
      .options = furrr::furrr_options(seed = TRUE)
    )
    
    # Process the parallel test results
    for (mutant_id in mutant_ids) {
      test_result <- parallel_results[[mutant_id]]
      pkg_copy_dir <- all_mutants[[mutant_id]]$pkg_copy_dir
      
      if (is.null(test_result)) {
        cat(sprintf("Mutant %s: Compilation failed, not included in results.\n", mutant_id))
        next
      }
      
      status <- if (test_result) "SURVIVED" else "KILLED"
      mutation_info <- all_mutants[[mutant_id]]$mutation_info
      
      cat(sprintf("Mutant %s: %s\n", mutant_id, status))
      cat(sprintf("Mutation info: %s\n", mutation_info))
      cat(sprintf("   Result: %s\n\n", status))
      
      package_mutants[[mutant_id]] <- list(
        path = pkg_copy_dir,
        mutation_info = mutation_info,
        result = test_result
      )
      test_results[[mutant_id]] <- test_result
    }
    
    # Clean up the parallel workers
    future::plan(future::sequential)
    
  } else {
    # Run tests sequentially if only one mutant is available.
    for (mutant_id in names(all_mutants)) {
      pkg_copy_dir <- all_mutants[[mutant_id]]$pkg_copy_dir
      test_result <- run_package_test(pkg_copy_dir)
      
      if (is.null(test_result)) {
        cat(sprintf("Mutant %s: Compilation failed, not included in results.\n", mutant_id))
        next
      }
      
      status <- if (test_result) "SURVIVED" else "KILLED"
      mutation_info <- all_mutants[[mutant_id]]$mutation_info
      
      cat(sprintf("Mutant %s: %s\n", mutant_id, status))
      cat(sprintf("Mutation info: %s\n", mutation_info))
      cat(sprintf("   Result: %s\n\n", status))
      
      package_mutants[[mutant_id]] <- list(
        path = pkg_copy_dir,
        mutation_info = mutation_info,
        result = test_result
      )
      test_results[[mutant_id]] <- test_result
    }
  }
  
  # Summarize test results.
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
