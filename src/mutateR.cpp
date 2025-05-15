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

#include <R.h>
#include <Rinternals.h>
#include "ASTHandler.hpp"
#include "Mutator.hpp"
#include <vector>

extern "C" SEXP C_mutate_single(SEXP expr_sexp, SEXP src_ref_sexp, bool is_inside_block)
{
    if (TYPEOF(expr_sexp) == EXPRSXP) {
        if (Rf_length(expr_sexp) == 0)
            Rf_error("EXPRSXP input has no expressions.");
        expr_sexp = VECTOR_ELT(expr_sexp, 0);
    }

    ASTHandler astHandler;
    std::vector<OperatorPos> operators =
        astHandler.gatherOperators(expr_sexp, src_ref_sexp, is_inside_block);

    const int n = static_cast<int>(operators.size());
    if (n == 0) {
        return Rf_allocVector(VECSXP, 0);     // no PROTECT needed – no alloc yet
    }

    Mutator mutator;

    // protect every mutant until we have copied it into the result list
    std::vector<SEXP> mutants;  mutants.reserve(n);
    int n_protected = 0;

    for (int i = 0; i < n; ++i) {
        auto result = mutator.applyMutation(expr_sexp, operators, i);
        auto mut = result.first;
        auto ok = result.second;
        if (ok) {
            // PROTECT(mut); ++n_protected;
            mutants.push_back(mut);
        }
    }

    const R_xlen_t m = static_cast<R_xlen_t>(mutants.size());
    SEXP res = PROTECT(Rf_allocVector(VECSXP, m)); ++n_protected;
    for (R_xlen_t i = 0; i < m; ++i)
        SET_VECTOR_ELT(res, i, mutants[i]);

    // UNPROTECT(n_protected);   // res is now reachable from R, others are inside res
    UNPROTECT(1 + m);         // res + every mutant
    return res;
}

static bool isValidMutant(SEXP mutant)
{
    int error = 0;
    R_tryEval(mutant, R_GlobalEnv, &error);
    return error == 0;
}

std::vector<bool> detect_block_expressions(SEXP exprs, int n_expr) {
    std::vector<bool> block_flags(n_expr, false);
    std::vector<bool> in_block_stack;  // Stack to track nested blocks
    
    for (int i = 0; i < n_expr; i++) {
        SEXP expr = VECTOR_ELT(exprs, i);
        
        // Check if the expression is a call (language object)
        if (TYPEOF(expr) == LANGSXP) {
            SEXP head = CAR(expr);
            if (TYPEOF(head) == SYMSXP) {
                std::string op_name = CHAR(PRINTNAME(head));
                if (op_name == "{") {
                    // Start of a new block
                    in_block_stack.push_back(true);
                    
                    // Process expressions inside this block
                    SEXP block_body = CDR(expr);
                    while (block_body != R_NilValue) {
                        block_flags[i] = true;
                        block_body = CDR(block_body);
                    }
                } else if (op_name == "}") {
                    // End of current block
                    if (!in_block_stack.empty()) {
                        in_block_stack.pop_back();
                    }
                } else if (!in_block_stack.empty()) {
                    // We're inside at least one block
                    block_flags[i] = true;
                }
            }
        }
        
        // Mark expression as in block if we're inside any block
        if (!in_block_stack.empty()) {
            block_flags[i] = true;
        }
    }
    
    return block_flags;
}

extern "C" SEXP C_mutate_file(SEXP exprs)
{
    if (TYPEOF(exprs) != EXPRSXP)
        Rf_error("Input must be an expression list (EXPRSXP).");

    SEXP src_ref = Rf_getAttrib(exprs, Rf_install("srcref"));
    if (TYPEOF(src_ref) != VECSXP || Rf_length(src_ref) != Rf_length(exprs))
        Rf_error("'srcref' attribute missing or malformed.");

    const int n_expr = Rf_length(exprs);
    std::vector<bool> inside_block = detect_block_expressions(exprs, n_expr);

    // we will collect *protected* mutants and unprotect them after transferring
    std::vector<SEXP> valid_mutants;
    int n_protected = 0;

    for (int i = 0; i < n_expr; ++i) {
        SEXP cur_expr     = VECTOR_ELT(exprs, i);
        SEXP cur_src_ref  = VECTOR_ELT(src_ref, i);

        SEXP cur_mutants  = C_mutate_single(cur_expr, cur_src_ref, inside_block[i]);
        if (TYPEOF(cur_mutants) != VECSXP)
            Rf_error("C_mutate_single did not return a list for expression %d.", i);

        const int n_mut   = Rf_length(cur_mutants);
        for (int j = 0; j < n_mut; ++j) {
            SEXP file_mut = PROTECT(Rf_allocVector(EXPRSXP, n_expr)); ++n_protected;
            SEXP mut_info = R_NilValue;

            for (int k = 0; k < n_expr; ++k) {
                if (k == i) {
                    SEXP mut = VECTOR_ELT(cur_mutants, j);
                    SET_VECTOR_ELT(file_mut, k, mut);
                    mut_info = Rf_getAttrib(mut, Rf_install("mutation_info"));
                } else {
                    SET_VECTOR_ELT(file_mut, k, VECTOR_ELT(exprs, k));
                }
            }
            Rf_setAttrib(file_mut, Rf_install("mutation_info"), mut_info);

            if (isValidMutant(file_mut))
                valid_mutants.push_back(file_mut); // still protected
            else {
                UNPROTECT(1); --n_protected;       // discard invalid mutant
            }
        }
    }

    // Build the final R list
    const R_xlen_t n_valid = static_cast<R_xlen_t>(valid_mutants.size());
    SEXP res = PROTECT(Rf_allocVector(VECSXP, n_valid)); ++n_protected;

    for (R_xlen_t i = 0; i < n_valid; ++i)
        SET_VECTOR_ELT(res, i, valid_mutants[i]);

    UNPROTECT(n_protected);   // drops all temporaries but keeps res reachable
    return res;
}

