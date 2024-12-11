dyn.load("mutateR.so")

mutate_file <- function(file_name) {
  dir.create("./mutations", showWarnings = FALSE)

  mutated_expressions <- .Call("C_generate_mutations", file_name)
  
  if (!is.list(mutated_expressions)) {
    stop("Error: Expected a list of mutated expressions.")
  }

  # Save each mutated version as a separate file
  for (i in seq_along(mutated_expressions)) {
    mutated_code <- deparse(mutated_expressions[[i]])
    output_file <- sprintf("./mutations/mutated_%03d.R", i)
    writeLines(mutated_code, output_file)
  }

  cat("Mutations saved successfully in './mutations' directory.\n")
}
