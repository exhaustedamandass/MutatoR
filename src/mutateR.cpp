#include <cpp11.hpp>
#include <R.h>
#include <Rinternals.h>
#include <Rembedded.h>
#include <R_ext/Parse.h>

// Undefine the 'length' macro defined by Rinternals.h to avoid conflicts with the C++ standard library
#undef length

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>
#include <vector>
#include <unordered_set>
#include <unordered_map>


// C_mutate_single.cpp
#include <R.h>
#include <Rinternals.h>
#include "ASTHandler.hpp"
#include "Mutator.hpp"
#include <vector>

// Function to Generate All Mutations for a Single SEXP

extern "C" SEXP C_mutate_single(SEXP expr_sexp) {
    if (TYPEOF(expr_sexp) != LANGSXP && TYPEOF(expr_sexp) != EXPRSXP) {
        Rf_error("Input must be an R expression (LANGSXP or EXPRSXP)");
    }

    // Handle EXPRSXP by taking the first element
    if (TYPEOF(expr_sexp) == EXPRSXP) {
        if (Rf_length(expr_sexp) < 1) {
            Rf_error("EXPRSXP input has no expressions.");
        }
        expr_sexp = VECTOR_ELT(expr_sexp, 0);
    }

    // Initialize ASTHandler and gather operators
    ASTHandler astHandler;
    std::vector<OperatorPos> operators = astHandler.gatherOperators(expr_sexp);

    int n = static_cast<int>(operators.size());
    if (n == 0) {
        Rf_warning("No operators found to mutate.");
        SEXP emptyList = PROTECT(Rf_allocVector(VECSXP, 0));
        UNPROTECT(1);
        return emptyList;
    }

    // Initialize Mutator
    Mutator mutator;

    // We'll store the mutated expressions in this vector
    std::vector<SEXP> mutatedExpressions;
    // Rough upper bound: each operator can mutate to 3 new operators if we have 4 total
    // =>  n * 3. That’s just an approximation for reserve().
    mutatedExpressions.reserve(n * 3);

    // For each operator, flip it to each alternative operator
    static std::vector<SEXP> allArithmeticOps = {
        Rf_install("+"),
        Rf_install("-"),
        Rf_install("*"),
        Rf_install("/")
    };

    for (int i = 0; i < n; i++) {
        SEXP originalSym = operators[i].op->getSymbol();
        for (SEXP candidateSym : allArithmeticOps) {
            if (candidateSym != originalSym) {
                // produce one mutant
                SEXP mutated = mutator.applySingleMutation(expr_sexp, operators, i, candidateSym);
                mutatedExpressions.push_back(mutated);
            }
        }
    }

    // Create an R list of all mutated expressions
    SEXP resultList = PROTECT(Rf_allocVector(VECSXP, mutatedExpressions.size()));
    for (size_t i = 0; i < mutatedExpressions.size(); i++) {
        SET_VECTOR_ELT(resultList, i, mutatedExpressions[i]);
    }

    UNPROTECT(1);
    return resultList;
}
