test_that("mutate_file_new creates mutations", {
  # Create a temporary R script for testing
  temp_file <- tempfile(fileext = ".R")
  writeLines("square <- function(x) {
    return(x * x)
  }
  
  add <- function(a, b) {
    return(a + b)
  }", temp_file)
  
  # Run mutation
  on.exit(unlink(file.path("mutations", basename(temp_file)), recursive = TRUE))
  mutated_files <- mutate_file_new(temp_file)
  
  # Check that mutations were created
  expect_true(is.list(mutated_files))
  expect_true(length(mutated_files) > 0)
  
  # Check that mutation files exist
  for (mutant in mutated_files) {
    expect_true(file.exists(mutant$path))
    expect_true(!is.null(mutant$mutation_info))
  }
  
  # Clean up
  unlink(temp_file)
  unlink("mutations", recursive = TRUE)
})

test_that("mutate_file_new handles empty files", {
  # Create an empty R script
  temp_file <- tempfile(fileext = ".R")
  writeLines("", temp_file)
  
  # Run mutation
  on.exit(unlink(file.path("mutations", basename(temp_file)), recursive = TRUE))
  mutated_files <- mutate_file_new(temp_file)
  
  # Only string-level deletion mutations should be attempted
  expect_true(is.list(mutated_files))
  
  # Clean up
  unlink(temp_file)
  unlink("mutations", recursive = TRUE)
}) 