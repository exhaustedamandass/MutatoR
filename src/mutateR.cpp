#include <cpp11.hpp>
#include <R.h>
#include <Rinternals.h>
#include <Rembedded.h>
#include <R_ext/Parse.h>
#include <R_ext/Print.h>

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
    // Initialize ASTHandler and gather operators
    // ASTHandler astHandler;
    // std::vector<OperatorPos> operators = astHandler.gatherOperators(expr_sexp);
    // Debug: Display the gathered operators in the console
    // std::cout << "Operators vector has " << operators.size() << " operator(s):" << std::endl;
    // for (size_t i = 0; i < operators.size(); i++) {
    //     const OperatorPos& op_pos = operators[i];
    //     std::cout << "Operator[" << i << "]:" << std::endl;
    //     std::cout << "  Path: ";
    //     for (const auto& idx : op_pos.path) {
    //         std::cout << idx << " ";
    //     }
    //     std::cout << std::endl;
    //     std::cout << "  Operator Type: " << op_pos.op->getType() << std::endl;
    //     std::cout << "  Start Position: (" << op_pos.start_line << ", " << op_pos.start_col << ")" << std::endl;
    //     std::cout << "  End Position: (" << op_pos.end_line << ", " << op_pos.end_col << ")" << std::endl;
    //     std::cout << "  Original Symbol: " << CHAR(PRINTNAME(op_pos.original_symbol)) << std::endl;
    // }

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
        //std::cout << TYPEOF(expr_sexp) << std::endl;
    }

    // Initialize ASTHandler and gather operators
    ASTHandler astHandler;
    std::vector<OperatorPos> operators = astHandler.gatherOperators(expr_sexp);


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
        SEXP mutated = mutator.applyMutation(expr_sexp, operators, i);
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

extern "C" SEXP C_mutate_file(SEXP exprs) {
    // Ensure the input is a list of expressions (EXPRSXP)
    if (TYPEOF(exprs) != EXPRSXP) {
        Rf_error("Input must be an expression list (EXPRSXP).");
    }
    
    int n_expr = Rf_length(exprs);
    std::vector<SEXP> all_mutants;
    
    // Loop over each expression in the file
    for (int i = 0; i < n_expr; i++) {
        SEXP cur_expr = VECTOR_ELT(exprs, i);
        // Get all mutants for the current expression
        SEXP cur_mutants = C_mutate_single(cur_expr);
        
        // Verify that the return is a list of mutants
        if (TYPEOF(cur_mutants) != VECSXP) {
            Rf_error("C_mutate_single did not return a list for expression %d.", i);
        }
        
        std::cout << "logging from C_mutate_file" << std::endl;

        Rf_PrintValue(cur_mutants);
        for (int idx = 0; idx < Rf_length(cur_mutants); idx++) {
            SEXP mutant = VECTOR_ELT(cur_mutants, idx);
            SEXP mutation_info = Rf_getAttrib(mutant, Rf_install("mutation_info"));
            if (mutation_info != R_NilValue) {
                Rf_PrintValue(mutation_info);
            }
        }

        int n_mutants = Rf_length(cur_mutants);
        // For each mutant of the current expression, create a complete file mutant.
        for (int j = 0; j < n_mutants; j++) {
            // Allocate a new EXPRSXP (list of expressions) for the mutated file.
            SEXP new_mutant_file = PROTECT(Rf_allocVector(EXPRSXP, n_expr));
            for (int k = 0; k < n_expr; k++) {
                // If this is the expression we mutated, use the mutant.
                if (k == i) {
                    SEXP mutant = VECTOR_ELT(cur_mutants, j);
                    SET_VECTOR_ELT(new_mutant_file, k, mutant);

                    // Reassign the mutation_info attribute
                    SEXP mutation_info = Rf_getAttrib(mutant, Rf_install("mutation_info"));
                    if (mutation_info != R_NilValue) {
                        std::cout << "adding mutation info attribute" << std::endl;
                        Rf_PrintValue(mutation_info);
                        Rf_setAttrib(VECTOR_ELT(new_mutant_file, k), Rf_install("mutation_info"), mutation_info);
                    }
                } else {
                    // Otherwise, copy the original expression.
                    SET_VECTOR_ELT(new_mutant_file, k, VECTOR_ELT(exprs, k));
                }
            }
            all_mutants.push_back(new_mutant_file);
            UNPROTECT(1);
        }
    }
    
    // Create an R list to hold all mutated file variants.
    SEXP resultList = PROTECT(Rf_allocVector(VECSXP, all_mutants.size()));
    for (size_t k = 0; k < all_mutants.size(); k++) {
        SET_VECTOR_ELT(resultList, k, all_mutants[k]);
        // TODO: from line 152, bring unprotect here
    }
    UNPROTECT(1);
    
    return resultList;
}
