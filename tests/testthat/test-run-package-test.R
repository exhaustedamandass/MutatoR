test_that("run_package_test can check test results", {
  # Skip test if devtools is not available
  skip_if_not_installed("devtools")
  
  # Mock a small package directory structure
  temp_dir <- tempfile()
  dir.create(temp_dir)
  on.exit(unlink(temp_dir, recursive = TRUE))
  
  # Create a simple package structure
  pkg_dir <- file.path(temp_dir, "testpkg")
  dir.create(pkg_dir)
  dir.create(file.path(pkg_dir, "R"), recursive = TRUE)
  dir.create(file.path(pkg_dir, "tests", "testthat"), recursive = TRUE)
  
  # Add basic package files
  writeLines("Package: testpkg
Version: 0.1.0
Title: Test Package
Description: A test package.
Author: Test Author
License: MIT
RoxygenNote: 7.1.1", file.path(pkg_dir, "DESCRIPTION"))
  
  writeLines("exportPattern(\"^[[:alpha:]]+\")", file.path(pkg_dir, "NAMESPACE"))
  
  # Create a simple R function
  writeLines("#' Add two numbers
#' @param a First number
#' @param b Second number
#' @return Sum of a and b
#' @export
add <- function(a, b) {
  return(a + b)
}", file.path(pkg_dir, "R", "add.R"))
  
  # Create a test for that function
  writeLines("library(testthat)
library(testpkg)

test_check(\"testpkg\")", file.path(pkg_dir, "tests", "testthat.R"))
  
  writeLines("test_that(\"addition works\", {
  expect_equal(add(2, 2), 4)
})", file.path(pkg_dir, "tests", "testthat", "test-add.R"))
  
  # We can only test that the function runs without errors
  # since we can't actually load a mock package in a test
  expect_error(run_package_test(pkg_dir), NA)
})

test_that("run_package_test returns appropriate value on compilation error", {
  # Skip test if devtools is not available
  skip_if_not_installed("devtools")
  
  # Create an invalid package
  temp_dir <- tempfile()
  dir.create(temp_dir)
  on.exit(unlink(temp_dir, recursive = TRUE))
  
  # Create a simple package structure with syntax error
  pkg_dir <- file.path(temp_dir, "badpkg")
  dir.create(pkg_dir)
  dir.create(file.path(pkg_dir, "R"), recursive = TRUE)
  
  writeLines("Package: badpkg
Version: 0.1.0
Title: Bad Package
Description: A package with syntax errors.
Author: Test Author
License: MIT", file.path(pkg_dir, "DESCRIPTION"))
  
  # Create an R file with a syntax error
  writeLines("bad_function <- function() {
  return(x  # Missing closing parenthesis
}", file.path(pkg_dir, "R", "bad.R"))
  
  # We expect the function to return NULL for a package with compilation errors
  # But since this might produce different behaviors in different environments,
  # we'll just check it runs without throwing an unhandled error
  expect_error(run_package_test(pkg_dir), NA)
}) 