# Utility: delete individual lines to create "string-deletion" mutants
delete_line_mutants <- function(src_file,
                                out_dir   = "mutations",
                                file_base = NULL,
                                max_del   = 5,
                                start_idx = 1) {
  if (is.null(file_base)) file_base <- basename(src_file)
  lines     <- readLines(src_file)
  non_empty <- which(nzchar(lines))
  count     <- min(max_del, length(lines))
  if (length(non_empty) == 0) {
    warning("No non-empty lines to delete.")
    return(list())
  }
  mutants <- vector("list", count)
  for (i in seq_len(count)) {
    idx      <- sample(non_empty, 1)
    out_file <- file.path(out_dir, sprintf("%s_%03d.R", file_base, start_idx + i - 1))
    writeLines(lines[-idx], out_file)
    mutants[[i]] <- list(
      path = out_file,
      info = sprintf("deleted line %d", idx)
    )
  }
  mutants
}

# Generate AST-based and line-deletion mutants for a single R file
mutate_file <- function(src_file, out_dir = "mutations") {
  dir.create(out_dir, showWarnings = FALSE)
  options(keep.source = TRUE)

  parsed <- parse(src_file, keep.source = TRUE)
  if (is.null(attr(parsed, "srcref"))) {
    attr(parsed, "srcref") <- lapply(parsed, function(x) c(1L,1L,1L,1L))
  }

  raw_mutations <- tryCatch(
    .Call("C_mutate_file", parsed),
    error = function(e) {
      message("C_mutate_file error: ", e$message)
      list()
    }
  )

  results   <- list()
  base_name <- basename(src_file)
  idx       <- 1L

  # AST-driven mutants
  for (m in raw_mutations) {
    if (!is.list(m) && !is.language(m)) next
    code <- tryCatch(
      vapply(m, function(x) {
        if (!is.language(x)) "" else paste(deparse(x), collapse = "\n")
      }, character(1)),
      error = function(e) NULL
    )
    if (length(code) == 0) next

    out_file <- file.path(out_dir, sprintf("%s_%03d.R", base_name, idx))
    writeLines(paste(code, collapse = "\n"), out_file)

    info <- attr(m, "mutation_info")
    if (is.null(info) || info == "") info <- "<no info>"

    results[[length(results) + 1]] <- list(path = out_file, info = info)
    idx <- idx + 1L
  }

  # Fallback string-deletion mutants
  results <- c(
    results,
    delete_line_mutants(src_file, out_dir, base_name,
                        max_del   = 5,
                        start_idx = length(results) + 1L)
  )

  results
}

# High-level: mutate every R file in a package, run tests in parallel, and summarize
mutate_package <- function(pkg_dir, cores = parallel::detectCores()) {
  r_files <- list.files(file.path(pkg_dir, "R"),
                        pattern   = "\\.R$",
                        full.names = TRUE)

  mutants <- list()
  for (src in r_files) {
    for (m in mutate_file(src)) {
      temp_root <- tempfile("mut_pkg_")
      pkg_copy   <- file.path(temp_root, basename(pkg_dir))
      dir.create(pkg_copy, recursive = TRUE)
      file.copy(pkg_dir, temp_root, recursive = TRUE)

      target <- file.path(pkg_copy, "R", basename(src))
      file.copy(m$path, target, overwrite = TRUE)

      id <- paste(basename(src), basename(m$path), sep = "_")
      mutants[[id]] <- list(pkg = pkg_copy, info = m$info)
    }
  }

  run_tests <- function(pkg_dir) {
    # Close any open graphics devices before running tests
    if (requireNamespace("grDevices", quietly = TRUE)) {
      while (grDevices::dev.cur() > 1) grDevices::dev.off()
    }
    old_wd <- getwd()
    on.exit({
      setwd(old_wd)
      if (requireNamespace("grDevices", quietly = TRUE)) {
        while (grDevices::dev.cur() > 1) grDevices::dev.off()
      }
    }, add = TRUE)
    setwd(pkg_dir)

    loaded <- tryCatch(
      { devtools::load_all(quiet = TRUE); TRUE },
      error = function(e) {
        message("Load error: ", e$message)
        FALSE
      }
    )
    if (!loaded) return(FALSE)

    passed <- tryCatch(
      {
        tr <- testthat::test_dir("tests/testthat", reporter = "silent")
        num_failed <- sum(tr$failed)
        num_failed == 0
      },
      error = function(e) {
        message("Test error: ", e$message)
        FALSE
      }
    )
    passed
  }

  # Set up parallel processing
  future::plan(future::multisession, 
               workers = min(cores, length(mutants)),
               earlySignal = TRUE)

  mutant_ids <- names(mutants)
  pkg_dirs <- sapply(mutants, function(x) x$pkg)
  pkg_dir_list <- setNames(as.list(pkg_dirs), mutant_ids)

  # Run tests in parallel with progress bar
  parallel_results <- furrr::future_map(
    pkg_dir_list,
    function(pkg) suppressMessages(suppressWarnings(run_tests(pkg))),
    .progress = TRUE,
    .options = furrr::furrr_options(seed = TRUE)
  )

  # Process the parallel test results
  package_mutants <- list()
  test_results <- list()
  for (mutant_id in mutant_ids) {
    test_result <- parallel_results[[mutant_id]]
    pkg_copy_dir <- mutants[[mutant_id]]$pkg

    if (is.null(test_result) || length(test_result) == 0) {
      cat(sprintf("Mutant %s: Compilation/test execution failed, marking as KILLED.\n", mutant_id))
      test_result <- FALSE
    }

    status <- if (isTRUE(test_result)) "SURVIVED" else "KILLED"
    mutation_info <- mutants[[mutant_id]]$info

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
  gc()  # Force garbage collection to clean up connections

  # Summarize test results
  total_mutants <- length(test_results)
  survived <- sum(unlist(test_results))
  killed <- total_mutants - survived
  mutation_score <- if (total_mutants > 0) (killed / total_mutants) * 100 else 0

  cat("\nMutation Testing Summary:\n")
  cat(sprintf("  Total mutants:    %d\n", total_mutants))
  cat(sprintf("  Killed:           %d\n", killed))
  cat(sprintf("  Survived:         %d\n", survived))
  cat(sprintf("  Mutation Score:   %.2f%%\n", mutation_score))

  invisible(list(package_mutants = package_mutants, test_results = test_results))
}