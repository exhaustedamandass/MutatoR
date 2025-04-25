test_that("mutate_package generates and tests mutants", {
  # Skip test if dependencies are not available
  skip_if_not_installed("devtools")
  skip_if_not_installed("furrr")
  skip_if_not_installed("future")
  
  # Create a simple test package
  temp_dir <- tempfile()
  dir.create(temp_dir)
  on.exit(unlink(temp_dir, recursive = TRUE))
  
  # Set up package structure
  pkg_name <- "testMutatoR"
  pkg_dir <- file.path(temp_dir, pkg_name)
  dir.create(pkg_dir)
  dir.create(file.path(pkg_dir, "R"), recursive = TRUE)
  dir.create(file.path(pkg_dir, "tests", "testthat"), recursive = TRUE)
  
  # Add basic package files
  writeLines(sprintf("Package: %s
Version: 0.1.0
Title: Test Package for MutatoR
Description: A test package for mutation testing.
Author: Test Author
License: MIT
RoxygenNote: 7.1.1", pkg_name), file.path(pkg_dir, "DESCRIPTION"))
  
  writeLines("exportPattern(\"^[[:alpha:]]+\")", file.path(pkg_dir, "NAMESPACE"))
  
  # Add a simple function to mutate
  writeLines("#' Calculate the absolute value
#' @param x A numeric value
#' @return The absolute value of x
#' @export
my_abs <- function(x) {
  if (x < 0) {
    return(-x)
  }
  return(x)
}", file.path(pkg_dir, "R", "my_abs.R"))
  
  # Add a test for the function
  writeLines(sprintf("library(testthat)
library(%s)

test_check(\"%s\")", pkg_name, pkg_name), file.path(pkg_dir, "tests", "testthat.R"))
  
  writeLines(sprintf("test_that(\"%s works\", {
  expect_equal(my_abs(-5), 5)
  expect_equal(my_abs(5), 5)
  expect_equal(my_abs(0), 0)
})", pkg_name), file.path(pkg_dir, "tests", "testthat", "test-my-abs.R"))
  
  # In a real test environment, we would test the full functionality
  # but for unit testing purposes, we'll mock the function to avoid
  # actually running all the tests, which would be time-consuming
  
  # Mock functions for testing
  with_mock(
    `MutatoR::mutate_file_new` = function(sample_file) {
      # Return mock mutated file info
      return(list(
        list(
          path = tempfile(fileext = ".R"),
          mutation_info = "Mock mutation: operator < changed to >"
        )
      ))
    },
    `MutatoR::run_package_test` = function(pkg_dir) {
      # Return a mock test result (TRUE = survived, FALSE = killed)
      return(FALSE)
    },
    {
      # Call the function with a small number of cores to speed up the test
      result <- mutate_package(pkg_dir, cores = 1)
      
      # Check the structure of the result
      expect_true(is.list(result))
      expect_true("package_mutants" %in% names(result))
      expect_true("test_results" %in% names(result))
    }
  )
}) 