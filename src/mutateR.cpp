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
    // Debug: Display the gathered operators in the console
    std::cout << "Operators vector has " << operators.size() << " operator(s):" << std::endl;
    for (size_t i = 0; i < operators.size(); i++) {
        const OperatorPos& op_pos = operators[i];
        std::cout << "Operator[" << i << "]:" << std::endl;
        std::cout << "  Path: ";
        for (const auto& idx : op_pos.path) {
            std::cout << idx << " ";
        }
        std::cout << std::endl;
        std::cout << "  Operator Type: " << op_pos.op->getType() << std::endl;
        std::cout << "  Start Position: (" << op_pos.start_line << ", " << op_pos.start_col << ")" << std::endl;
        std::cout << "  End Position: (" << op_pos.end_line << ", " << op_pos.end_col << ")" << std::endl;
        std::cout << "  Original Symbol: " << CHAR(PRINTNAME(op_pos.original_symbol)) << std::endl;
    }

    

    // Debug: Print operators to the console
    int n = static_cast<int>(operators.size());
    if (n == 0) {
        Rf_warning("No operators found to mutate.");
        SEXP emptyList = PROTECT(Rf_allocVector(VECSXP, 0));
        UNPROTECT(1);
        return emptyList;
    }

    // Initialize Mutator
    Mutator mutator;

    // We'll store the mutated expressions in this vector: one
    // mutant per operator.
    std::vector<SEXP> mutatedExpressions;
    mutatedExpressions.reserve(n);

    // For each operator, apply its specific flip method.
    for (int i = 0; i < n; i++) {
        SEXP mutated = mutator.applyFlipMutation(expr_sexp, operators, i);
        mutatedExpressions.push_back(mutated);
    }

    // Create an R list of all mutated expressions
    SEXP resultList = PROTECT(Rf_allocVector(VECSXP, mutatedExpressions.size()));
    for (size_t i = 0; i < mutatedExpressions.size(); i++) {
        SET_VECTOR_ELT(resultList, i, mutatedExpressions[i]);
    }

    UNPROTECT(1);
    return resultList;
}
