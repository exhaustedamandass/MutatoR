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
extern "C" SEXP C_mutate_single(SEXP expr_sexp, SEXP src_ref_sexp, bool is_inside_block) {
    // if (TYPEOF(expr_sexp) != LANGSXP && TYPEOF(expr_sexp) != EXPRSXP && TYPEOF(expr_sexp) != NILSXP) {
    //     Rf_error("Input must be an R expression (LANGSXP or EXPRSXP) This is %s", TYPEOF(expr_sexp));
    // }

    // Handle EXPRSXP by taking the first element
    if (TYPEOF(expr_sexp) == EXPRSXP) {
        if (Rf_length(expr_sexp) < 1) {
            Rf_error("EXPRSXP input has no expressions.");
        }
        expr_sexp = VECTOR_ELT(expr_sexp, 0);
    }

    // Initialize ASTHandler and gather operators
    ASTHandler astHandler;
    std::vector<OperatorPos> operators = astHandler.gatherOperators(expr_sexp, src_ref_sexp, is_inside_block);

    // Debug: Print operators to the console
    int n = static_cast<int>(operators.size());
    if (n == 0) {
        // Rf_warning("No operators found to mutate.");
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
        std::pair<SEXP, bool> result = mutator.applyMutation(expr_sexp, operators, i);
        if(result.second)
            mutatedExpressions.push_back(result.first);
    }

    // Create an R list of all mutated expressions
    SEXP resultList = PROTECT(Rf_allocVector(VECSXP, mutatedExpressions.size()));
    for (size_t i = 0; i < mutatedExpressions.size(); i++) {
        SET_VECTOR_ELT(resultList, i, mutatedExpressions[i]);
    }

    UNPROTECT(1);
    return resultList;
}

bool isValidMutant(SEXP mutant) {
    SEXP compiled_result = R_NilValue;
    PROTECT(compiled_result = Rf_eval(mutant, R_GlobalEnv)); // Evaluate in the global environment

    bool is_valid = (compiled_result != R_NilValue); // Check if the evaluation was successful
    UNPROTECT(1); // Unprotect the compiled_result
    return is_valid;
}

std::vector<bool> detect_block_expressions(SEXP exprs, int n_expr) {
  std::vector<bool> block_flags(n_expr, false);
  
  for (int i = 0; i < n_expr; i++) {
    SEXP expr = VECTOR_ELT(exprs, i);
    
    // Check if the expression is a call (language object)
    if (TYPEOF(expr) == LANGSXP) {
      SEXP head = CAR(expr);
      if (TYPEOF(head) == SYMSXP) {
        std::string op_name = CHAR(PRINTNAME(head));
        block_flags[i] = (op_name == "{");
      }
    }
  }
  
  return block_flags;
}

extern "C" SEXP C_mutate_file(SEXP exprs) {
    PROTECT(exprs);
    int nProtected = 1;

    if (TYPEOF(exprs) != EXPRSXP) {
        Rf_error("Input must be an expression list (EXPRSXP).");
    }

    SEXP src_ref = Rf_getAttrib(exprs, Rf_install("srcref"));
    int n_expr = Rf_length(exprs);

    std::vector<bool> is_inside_block = detect_block_expressions(exprs, n_expr);

    std::vector<SEXP> all_mutants;

    // Loop over each expression in the file
    for (int i = 0; i < n_expr; i++) {
        SEXP cur_expr = VECTOR_ELT(exprs, i);
        SEXP cur_src_ref = VECTOR_ELT(src_ref, i);

        // PROTECT the return from C_mutate_single
        SEXP cur_mutants = PROTECT(C_mutate_single(cur_expr, cur_src_ref, is_inside_block[i]));
        nProtected++;  // we just PROTECTed cur_mutants

        if (TYPEOF(cur_mutants) != VECSXP) {
            Rf_error("C_mutate_single did not return a list for expression %d.", i);
        }

        int n_mutants = Rf_length(cur_mutants);
        for (int j = 0; j < n_mutants; j++) {
            SEXP new_mutant_file = PROTECT(Rf_allocVector(EXPRSXP, n_expr));
            nProtected++;

            // Fill new_mutant_file ...
            // e.g. copy the original expressions, except the jth one is replaced
            // with the mutant version
            for (int k = 0; k < n_expr; k++) {
                if (k == i) {
                    SEXP mutant = VECTOR_ELT(cur_mutants, j);
                    SET_VECTOR_ELT(new_mutant_file, k, mutant);
                    // mutation info etc.
                } else {
                    SET_VECTOR_ELT(new_mutant_file, k, VECTOR_ELT(exprs, k));
                }
            }
            all_mutants.push_back(new_mutant_file);
        }

        // Now that we have processed all mutants in cur_mutants,
        // we can unprotect *just cur_mutants*:
        UNPROTECT(1);
        nProtected--;
    }

    // Now we have a big vector of all_mutants.
    // Next we create a list to hold them
    SEXP resultList = PROTECT(Rf_allocVector(VECSXP, all_mutants.size()));
    nProtected++;

    size_t valid_count = 0;
    for (size_t k = 0; k < all_mutants.size(); k++) {
        // isValidMutant() calls Rf_eval, so we must keep them PROTECTed
        if (isValidMutant(all_mutants[k])) {
            SET_VECTOR_ELT(resultList, valid_count, all_mutants[k]);
            valid_count++;
        }
    }

    // Trim resultList
    SEXP final_resultList = PROTECT(Rf_allocVector(VECSXP, valid_count));
    nProtected++;
    for (size_t i = 0; i < valid_count; i++) {
        SET_VECTOR_ELT(final_resultList, i, VECTOR_ELT(resultList, i));
    }

    // Once final_resultList is allocated and protected, 
    // it now safely contains all the valid mutant objects.
    // We can unprotect everything at once now.
    UNPROTECT(nProtected);

    return final_resultList;
}

