#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

// Declare the function
extern SEXP C_mutate_single(SEXP expr_sexp);

// Define the registration table
static const R_CallMethodDef CallEntries[] = {
    {"C_mutate_single", (DL_FUNC) &C_mutate_single, 1},  // Function name, pointer, and number of arguments
    {NULL, NULL, 0}
};

// Register the functions
void R_init_MutatoR(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}