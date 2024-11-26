dyn.load("mutateR.so")

# R function to mutate the R file
mutate_file <- function(file_name) {
  # Call the C++ function "C_mutate" defined in mutateR.cpp
  mutated_code <- .Call("C_mutate", file_name)
  
  # Write the mutated content back to the file
  writeLines(mutated_code, file_name)
  
  # Inform the user
  cat("File mutated successfully:", file_name, "\n")
}
