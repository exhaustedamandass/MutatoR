library(testthat)

test_that("Arithmetic operations are correct", {
  expect_equal(result_add, 3)
  expect_equal(result_sub, 2)
  expect_equal(result_mul, 8)
  expect_equal(result_div, 4)
})

test_that("Logical operations are correct", {
  expect_false(result_and)
  expect_true(result_or)
})

test_that("Comparison operations are correct", {
  expect_true(result_eq)
  expect_true(result_neq)
  expect_true(result_lt)
  expect_true(result_gt)
  expect_true(result_le)
  expect_true(result_ge)
})

test_that("Complex expression evaluates correctly", {
  expect_equal(result_complex, 1)
})