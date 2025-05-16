# Utility: delete individual lines to create "string-deletion" mutants
delete_line_mutants <- function(src_file,
                                out_dir   = "mutations",
                                file_base = NULL,
                                max_del   = 5,
                                start_idx = 1) {
  if (is.null(file_base)) file_base <- basename(src_file)
  lines     <- readLines(src_file)
  
  # Filter out empty lines and comment lines
  non_empty <- which(nzchar(lines))
  non_comment <- which(!grepl("^\\s*#", lines))
  
  # Only keep lines that are both non-empty and non-comments
  valid_lines <- intersect(non_empty, non_comment)
  
  count <- min(max_del, length(valid_lines))
  if (length(valid_lines) == 0) {
    warning("No valid lines to delete (all lines are empty or comments).")
    return(list())
  }
  
  mutants <- vector("list", count)
  for (i in seq_len(count)) {
    idx      <- sample(valid_lines, 1)
    out_file <- file.path(out_dir, sprintf("%s_%03d.R", file_base, start_idx + i - 1))
    writeLines(lines[-idx], out_file)
    mutants[[i]] <- list(
      path = out_file,
      info = sprintf("deleted line %d", idx)
    )
  }
  mutants
}

#' Identify equivalent mutants using OpenAI API
#'
#' Analyzes survived mutants to determine if they are functionally equivalent
#' to the original code using OpenAI's language models.
#'
#' @param src_file Path to the original source file
#' @param survived_mutants List of mutants that survived test execution
#' @param api_config Optional API configuration (will be loaded if NULL)
#'
#' @return Updated list of survived mutants with equivalence information
identify_equivalent_mutants <- function(src_file, survived_mutants, api_config = NULL) {
  # Load API configuration if not provided
  if (is.null(api_config)) {
    api_config <- get_openai_config()
  }
  
  # If no API key is available, return early
  if (is.null(api_config$api_key) || api_config$api_key == "") {
    warning("OpenAI API key not found. Skipping equivalent mutant detection.")
    return(survived_mutants)
  }
  
  # Read original source code
  orig_code <- paste(readLines(src_file), collapse = "\n")
  
  # Group mutants by source file
  mutants_by_file <- list()
  for (id in names(survived_mutants)) {
    file_name <- strsplit(id, "_")[[1]][1]
    if (is.null(mutants_by_file[[file_name]])) {
      mutants_by_file[[file_name]] <- list()
    }
    mutants_by_file[[file_name]][[id]] <- survived_mutants[[id]]
  }
  
  # Track counts for each category
  equiv_count <- 0
  not_equiv_count <- 0
  unknown_count <- 0
  
  # Process each source file
  for (file_name in names(mutants_by_file)) {
    file_mutants <- mutants_by_file[[file_name]]
    
    # Prepare mutant information for the prompt
    mutant_details <- lapply(names(file_mutants), function(mid) {
      list(
        id = mid,
        mutation_info = file_mutants[[mid]]$mutation_info
      )
    })
    
    # Create the prompt
    prompt <- create_equivalent_mutant_prompt(orig_code, mutant_details)
    
    cat("\nAnalyzing mutants with OpenAI API...\n")
    cat("Prompt being sent to OpenAI:\n")
    cat("----------------------------------------\n")
    cat(prompt)
    cat("\n----------------------------------------\n\n")
    
    # Call OpenAI API
    response <- call_openai_api(prompt, api_config)
    
    # Process response
    if (!is.null(response)) {
      parsed <- response
      
      cat("Answer received from OpenAI API\n")
      cat("----------------------------------------\n")
      cat(parsed$choices[[1]]$message$content)
      cat("\n----------------------------------------\n\n")
      
      if (!is.null(parsed$choices) && length(parsed$choices) > 0) {
        # Extract equivalent mutants information from response
        equivalent_analysis <- parsed$choices[[1]]$message$content
        
        # Update the mutants with equivalence information
        for (mid in names(file_mutants)) {
          if (grepl(paste0(mid, ".*EQUIVALENT"), equivalent_analysis, ignore.case = TRUE) && 
              !grepl(paste0(mid, ".*NOT EQUIVALENT"), equivalent_analysis, ignore.case = TRUE)) {
            survived_mutants[[mid]]$equivalent <- TRUE
            survived_mutants[[mid]]$equivalence_status <- "EQUIVALENT"
            equiv_count <- equiv_count + 1
            cat(sprintf("Mutant %s identified as EQUIVALENT\n", mid))
          } else if (grepl(paste0(mid, ".*NOT EQUIVALENT"), equivalent_analysis, ignore.case = TRUE)) {
            survived_mutants[[mid]]$equivalent <- FALSE
            survived_mutants[[mid]]$equivalence_status <- "NOT EQUIVALENT"
            not_equiv_count <- not_equiv_count + 1
            cat(sprintf("Mutant %s identified as NOT EQUIVALENT\n", mid))
          } else if (grepl(paste0(mid, ".*DONT KNOW"), equivalent_analysis, ignore.case = TRUE)) {
            survived_mutants[[mid]]$equivalent <- NA
            survived_mutants[[mid]]$equivalence_status <- "DONT KNOW"
            unknown_count <- unknown_count + 1
            cat(sprintf("Mutant %s: DONT KNOW\n", mid))
          } else {
            # Default to unknown if no clear determination
            survived_mutants[[mid]]$equivalent <- NA
            survived_mutants[[mid]]$equivalence_status <- "DONT KNOW"
            unknown_count <- unknown_count + 1
            cat(sprintf("Mutant %s: No clear determination, marking as DONT KNOW\n", mid))
          }
        }
      }
    }
  }
  
  cat("\nEquivalence Analysis Summary:\n")
  cat(sprintf("  Equivalent:     %d\n", equiv_count))
  cat(sprintf("  Not Equivalent: %d\n", not_equiv_count))
  cat(sprintf("  Uncertain:      %d\n", unknown_count))
  cat("\n")
  
  return(survived_mutants)
}

#' Create a prompt for equivalent mutant detection
#'
#' Generates a well-formatted prompt for the OpenAI API to analyze
#' if mutants are equivalent to the original code.
#'
#' @param original_code String containing the original source code
#' @param mutant_details List of mutant details including IDs and mutation info
#'
#' @return A formatted prompt string for the OpenAI API
create_equivalent_mutant_prompt <- function(original_code, mutant_details) {
  mutant_info <- paste(sapply(mutant_details, function(m) {
    paste0("Mutant ID: ", m$id, "\nMutation: ", m$mutation_info, "\n")
  }), collapse = "\n")
  
  prompt <- paste0(
    "Determine if the following mutants are equivalent to the original code. ",
    "An equivalent mutant has the same behavior as the original code under all ",
    "possible inputs.\n\n",
    "Be conservative in your assessment. For each mutant, respond with one of these options:\n",
    "- 'EQUIVALENT': Only if you are certain the mutant is functionally identical to the original code\n",
    "- 'NOT EQUIVALENT': Only if you are certain the mutant changes behavior for some inputs\n",
    "- 'DONT KNOW': If you are uncertain or cannot determine equivalence\n\n",
    "Only answer with certainty if you are sure. If there's any doubt, use 'DONT KNOW'.\n\n",
    "Original code:\n```\n", original_code, "\n```\n\n",
    "Survived mutants:\n", mutant_info
  )
  
  return(prompt)
}

#' Call OpenAI API
#'
#' Makes a POST request to the OpenAI Chat Completions API.
#'
#' @param prompt The prompt to send to the API
#' @param config API configuration with key and model information
#'
#' @return API response as text, or NULL if request failed
call_openai_api <- function(prompt, config) {
  tryCatch({
    # Create the request body without temperature parameter
    request_body <- list(
      model = config$model,
      messages = list(
        list(
          role = "system",
          content = paste0(
            "You are an expert in program analysis, ",
            "particularly in identifying equivalent mutants in code."
          )
        ),
        list(
          role = "user",
          content = prompt
        )
      )
    )
    
    # Convert to JSON with proper settings
    json_body <- jsonlite::toJSON(request_body, auto_unbox = TRUE)
    
    # Make the API request
    response <- httr::POST(
      url = "https://api.openai.com/v1/chat/completions",
      httr::add_headers(
        "Content-Type" = "application/json",
        "Authorization" = paste("Bearer", config$api_key)
      ),
      body = json_body,
      encode = "json"
    )
    
    # Check for HTTP errors
    httr::stop_for_status(response)
    
    if (httr::status_code(response) == 200) {
      return(httr::content(response, as = "parsed", type = "application/json"))
    } else {
      warning(
        "OpenAI API error: ", 
        httr::content(response, "text", encoding = "UTF-8")
      )
      return(NULL)
    }
  }, error = function(e) {
    warning("Error calling OpenAI API: ", e$message)
    return(NULL)
  })
}

#' Get OpenAI API configuration
#'
#' Retrieves API key and model configuration from environment variables
#' or a configuration file.
#'
#' @return List containing api_key and model values
get_openai_config <- function() {
  api_key <- Sys.getenv("OPENAI_API_KEY", "")
  model   <- Sys.getenv("OPENAI_MODEL",   "gpt-4")

  if (api_key == "") {
    # candidates: .openai_config.R and .openai_config.R.template
    candidates <- c(".openai_config.R", ".openai_config.R.template")
    wd         <- normalizePath(getwd())
    config_path <- NULL

    # walk up until we hit root
    repeat {
      for (f in candidates) {
        p <- file.path(wd, f)
        if (file.exists(p)) {
          config_path <- p
          break
        }
      }
      if (!is.null(config_path)) break
      parent <- dirname(wd)
      if (parent == wd) break
      wd <- parent
    }

    if (!is.null(config_path)) {
      config_env <- new.env()
      try(source(config_path, local = config_env), silent = TRUE)

      # pick up either lowercase or uppercase var names
      if (exists("api_key",          envir = config_env)) api_key <- get("api_key",          envir = config_env)
      if (exists("OPENAI_API_KEY",   envir = config_env)) api_key <- get("OPENAI_API_KEY",   envir = config_env)
      if (exists("model",            envir = config_env)) model   <- get("model",            envir = config_env)
      if (exists("OPENAI_MODEL",     envir = config_env)) model   <- get("OPENAI_MODEL",     envir = config_env)
    }
  }

  list(api_key = api_key, model = model)
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
mutate_package <- function(pkg_dir, cores = parallel::detectCores(), 
                           isFullLog = FALSE, detectEqMutants = FALSE) {
  ## 1. clean up previous leftovers right at the start
  unlink(list.files(tempdir(), "^mut_pkg_", full.names = TRUE), recursive = TRUE)
                           
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
               gc      = TRUE,          # Enable garbage collection after each result
               earlySignal = TRUE)

  mutant_ids <- names(mutants)
  pkg_dirs <- sapply(mutants, function(x) x$pkg)
  pkg_dir_list <- setNames(as.list(pkg_dirs), mutant_ids)

  # Run tests in parallel with progress bar
  parallel_results <- furrr::future_map(
    mutant_ids,
    function(id) {
      dir <- pkg_dir_list[[id]]
      out <- suppressMessages(suppressWarnings(run_tests(dir)))
      unlink(dir, recursive = TRUE, force = TRUE)   # free disk immediately
      out
    },
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

    if (isFullLog) {
      cat(sprintf("Mutant %s: %s\n", mutant_id, status))
      cat(sprintf("Mutation info: %s\n", mutation_info))
      cat(sprintf("   Result: %s\n\n", status))
    }

    package_mutants[[mutant_id]] <- list(
      path = pkg_copy_dir,
      mutation_info = mutation_info,
      result = test_result
    )
    test_results[[mutant_id]] <- test_result
  }

  # Filter survived mutants
  survived_mutants <- package_mutants[unlist(test_results)]
  
  # Initialize counters
  equivalent <- 0
  not_equivalent <- 0
  uncertain <- 0
  
  # Identify equivalent mutants among survived mutants only if detectEqMutants is TRUE
  if (detectEqMutants && length(survived_mutants) > 0) {
    cat("\nAnalyzing equivalent mutants among survived mutants...\n")
    # Get the original source files for survived mutants
    src_files <- unique(sapply(names(survived_mutants), function(id) {
      file_name <- strsplit(id, "_")[[1]][1]
      file.path(pkg_dir, "R", file_name)
    }))
    
    # Process each source file
    for (src_file in src_files) {
      # Get mutants for this source file
      file_mutants <- survived_mutants[grep(basename(src_file), names(survived_mutants))]
      if (length(file_mutants) > 0) {
        file_mutants <- identify_equivalent_mutants(src_file, file_mutants)
        # Update the main package_mutants list with equivalence information
        for (id in names(file_mutants)) {
          package_mutants[[id]]$equivalent <- file_mutants[[id]]$equivalent
          if (!is.null(file_mutants[[id]]$equivalence_status)) {
            package_mutants[[id]]$equivalence_status <- file_mutants[[id]]$equivalence_status
          }
        }
      }
    }
  }

  # Clean up the parallel workers
  future::plan(future::sequential)
  gc()  # Force garbage collection to clean up connections

  # Summarize test results
  total_mutants <- length(test_results)
  survived <- sum(unlist(test_results))
  killed <- total_mutants - survived
  
  # Calculate equivalent mutants only if detectEqMutants is TRUE
  if (detectEqMutants) {
    equivalent <- sum(sapply(package_mutants, function(m) isTRUE(m$equivalent)), na.rm = TRUE)
    not_equivalent <- sum(sapply(package_mutants, function(m) isFALSE(m$equivalent)), na.rm = TRUE)
    uncertain <- sum(sapply(package_mutants, function(m) is.na(m$equivalent) && !is.null(m$equivalent)), na.rm = TRUE)
  }
  
  adjusted_survived <- survived - equivalent
  mutation_score <- if (total_mutants > 0) 
    (killed / total_mutants) * 100 else 0
    
  adjusted_mutation_score <- if (total_mutants - equivalent > 0) 
    (killed / (total_mutants - equivalent)) * 100 else 0

  cat("\nMutation Testing Summary:\n")
  cat(sprintf("  Total mutants:    %d\n", total_mutants))
  cat(sprintf("  Killed:           %d\n", killed))
  cat(sprintf("  Survived:         %d\n", survived))
  
  # Only print equivalent mutants and adjusted score if detectEqMutants is TRUE
  if (detectEqMutants) {
    cat(sprintf("  Equivalent:       %d\n", equivalent))
    cat(sprintf("  Not Equivalent:   %d\n", not_equivalent))
    cat(sprintf("  Uncertain:        %d\n", uncertain))
    cat(sprintf("  Mutation Score:   %.2f%%\n", mutation_score))
    cat(sprintf("  Adjusted Score:   %.2f%% (excluding equivalent mutants)\n", adjusted_mutation_score))
  } else {
    cat(sprintf("  Mutation Score:   %.2f%%\n", mutation_score))
  }

  invisible(list(package_mutants = package_mutants, test_results = test_results))
}