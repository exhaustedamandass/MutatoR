library(testthat)
library(dplyr)
library(lubridate)

# Sample data for testing
test_data <- data.frame(
  track_id = c("id1", "id1", "id2", "id3"),
  track_name = c("Song A", "Song A", "Song B", "Song C"),
  track_popularity = c(60, 60, 20, 40),
  track_album_release_date = c("2020-01-01", "2020-01-01", "2021-01-01", "2019-05-05"),
  playlist_genre = c("Pop", "pop", "Rock", "Pop"),
  danceability = c(0.85, 0.75, 0.95, 0.65),
  duration_ms = c(230000, 240000, 220000, 250000),
  stringsAsFactors = FALSE
)

# Set constants for filtering criteria
MIN_TRACK_POPULARITY <- 35
TARGET_PLAYLIST_GENRE <- "pop"
MAX_DANCEABILITY <- 0.8
MAX_DURATION_MS <- 234680

# 6. Test: Filtering Steps
test_that("data is filtered correctly by popularity, genre, danceability, and duration", {
  filtered_data <- test_data %>%
    filter(track_popularity > MIN_TRACK_POPULARITY) %>%
    filter(tolower(playlist_genre) == TARGET_PLAYLIST_GENRE) %>%
    filter(danceability < MAX_DANCEABILITY) %>%
    filter(duration_ms < MAX_DURATION_MS) 
  
  # Verify that only the row meeting all criteria remains
  expect_equal(nrow(filtered_data), 1)
  expect_true(filtered_data$track_id == "id1")  # Only "id1" meets all criteria
})

# Run the tests
test_file("test_data_cleaning.R")


# # 1. Test: Remove Duplicate Rows
# test_that("duplicates are removed based on track_id", {
#   deduped_data <- test_data %>%
#     distinct(track_id, .keep_all = TRUE)
#   expect_equal(nrow(deduped_data), 3)  # Expect 3 rows after removing duplicates
# })

# # 2. Test: Convert `track_album_release_date` to Date Format
# test_that("track_album_release_date is converted to Date", {
#   converted_data <- test_data %>%
#     mutate(track_album_release_date = ymd(track_album_release_date))
#   expect_true(all(class(converted_data$track_album_release_date) == "Date"))
# })

# # 3. Test: Normalize Numeric Columns
# test_that("numeric columns are normalized correctly", {
#   numeric_columns <- c("track_popularity", "danceability")
#   normalized_data <- test_data %>%
#     mutate(across(all_of(numeric_columns), scale))
#   expect_equal(mean(normalized_data$track_popularity, na.rm = TRUE), 0, tolerance = 1e-5)
#   expect_equal(mean(normalized_data$danceability, na.rm = TRUE), 0, tolerance = 1e-5)
# })

# # 4. Test: Standardize Text in Categorical Columns
# test_that("text columns are standardized to lowercase", {
#   standardized_data <- test_data %>%
#     mutate(playlist_genre = tolower(playlist_genre))
#   expect_true(all(standardized_data$playlist_genre == c("pop", "pop", "rock", "pop")))
# })

# # 5. Test: Drop Unnecessary Columns
# test_that("unnecessary columns are dropped", {
#   columns_to_drop <- c("track_name")
#   dropped_data <- test_data %>%
#     select(-all_of(columns_to_drop))
#   expect_false("track_name" %in% colnames(dropped_data))
# })


cat("Sum:", sum, "\n")

# Subtraction
difference <- a - b
cat("Difference:", difference, "\n")

# Multiplication
product <- a * b
cat("Product:", product, "\n")

# Division
quotient <- a / b
cat("Quotient:", quotient, "\n")

# Modulus (remainder)
remainder <- a %% b
cat("Remainder:", remainder, "\n")

# If-else statement with comparison operators
if (a > b) {
  cat("a is greater than b\n")
} else if (a < b) {
  cat("a is less than b\n")
} else {
  cat("a is equal to b\n")
}

# Using another comparison with equality check
x <- 15
y <- 20

if (x == y) {
  cat("x is equal to y\n")
} else {
  cat("x is not equal to y\n")
}

# Nested if-else
z <- 25
if (z > 10) {
  if (z < 30) {
    cat("z is between 10 and 30\n")
  } else {
    cat("z is 30 or more\n")
  }
} else {
  cat("z is 10 or less\n")
}
