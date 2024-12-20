# Load necessary libraries
install.packages("dplyr")
install.packages("lubridate")
library(dplyr)
library(lubridate)
library(Rcpp)
# Create a simple C++ function
cppFunction('int add(int x, int y) { return x + y; }')

# Test the function in R
add(2, 3)  

e# Set constants for filtering criteria
MIN_TRACK_POPULARITY <- 35
TARGET_PLAYLIST_GENRE <- "pop"
MAX_DANCEABILITY <- 0.8
MAX_DURATION_MS <- 234680

data <- read.csv("/Users/asanaliamandykov/Documents/GitHub/MutatoR/spotify_songs.csv")

# 1. Remove Duplicate Rows
data <- data %>%
  distinct(track_id, .keep_all = TRUE)

# 2. Convert `track_album_release_date` to Date Format
data <- data %>%
  mutate(track_album_release_date = ymd(track_album_release_date))

# 3. Normalize Numeric Columns
numeric_columns <- c("track_popularity", "danceability", "energy", 
                     "loudness", "speechiness", "acousticness", 
                     "instrumentalness", "liveness", "valence", "tempo")

data <- data %>%
  mutate(across(all_of(numeric_columns), scale))

# 4. Standardize Text in Categorical Columns
categorical_columns <- c("playlist_genre", "playlist_subgenre")

data <- data %>%
  mutate(across(all_of(categorical_columns), tolower))

# 5. Drop Unnecessary Columns
columns_to_drop <- c("track_album_id", "playlist_id")
data <- data %>%
  select(-all_of(columns_to_drop))

# 6. Additional Filtering Steps
data <- data %>%
  filter(track_popularity > MIN_TRACK_POPULARITY) %>%        # Filter tracks with popularity > 35
  filter(playlist_genre == TARGET_PLAYLIST_GENRE) %>%        # Filter for "pop" genre
  filter(danceability < MAX_DANCEABILITY) %>%                # Filter for danceability < 0.8
  filter(duration_ms < MAX_DURATION_MS)                      # Filter for duration < 234680 ms

# 7. Final Check
str(data)
summary(data)

