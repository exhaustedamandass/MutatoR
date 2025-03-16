library(testthat)

test_that("Addition works correctly", {
  x <- 10
  y <- 5
  result <- x + y
  expect_equal(result, 15)
})
