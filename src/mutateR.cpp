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


struct OperatorPos {
    std::vector<int> path;    // Path indices from root to reach this operator
    SEXP operator_symbol;     // + or -
};

// Recursively gather all operators (+ and -) and their locations in the AST
std::vector<OperatorPos> gather_operators(SEXP expr, std::vector<int> path = {}) {
    std::vector<OperatorPos> ops;
    if (TYPEOF(expr) == LANGSXP) {
        SEXP fun = CAR(expr);
        if (fun == Rf_install("+") || fun == Rf_install("-")) {
            //CHAR(PRINTNAME(fun))
            OperatorPos op;
            op.path = path;
            op.operator_symbol = fun;
            ops.push_back(op);
        }

        int i = 0;
        for (SEXP next = CDR(expr); next != R_NilValue; next = CDR(next), i++) {
            auto child_path = path;
            child_path.push_back(i);
            auto child_ops = gather_operators(CAR(next), child_path);
            ops.insert(ops.end(), child_ops.begin(), child_ops.end());
        }
    }
    return ops;
}

// Apply a given subset of operator flips to the original expression
SEXP apply_mutations(SEXP expr, const std::vector<OperatorPos>& ops, int mask) {
    SEXP mutated = Rf_duplicate(expr);
    PROTECT(mutated);

    for (int i = 0; i < (int)ops.size(); i++) {
        if (mask & (1 << i)) {
            // Flip this operator
            const auto& op = ops[i];
            SEXP node = mutated;

            // Navigate down the path to find the operator node
            for (int idx : op.path) {
                SEXP nxt = CDR(node);
                for (int j = 0; j < idx; j++) {
                    nxt = CDR(nxt);
                }
                node = CAR(nxt);
            }

            // Flip the operator
            SEXP old_fun = CAR(node);
            if (old_fun == Rf_install("+")) {
                SETCAR(node, Rf_install("-"));
            } else {
                SETCAR(node, Rf_install("+"));
            }
        }
    }

    UNPROTECT(1);
    return mutated;
}

// Generate all mutated versions of the expression by flipping subsets of operators
std::vector<SEXP> generate_all_mutations(SEXP expr) {
    std::vector<SEXP> results;
    auto ops = gather_operators(expr);

    // There are 2^n - 1 subsets of operators (excluding the empty subset)
    int n = (int)ops.size();
    for (int mask = 1; mask < (1 << n); mask++) {
        SEXP mutated = apply_mutations(expr, ops, mask);
        results.push_back(mutated);
    }

    return results;
}

extern "C" SEXP C_generate_mutations(SEXP file_name) {
    if (TYPEOF(file_name) != STRSXP || Rf_length(file_name) < 1) {
        Rf_error("Input must be a single string (file name).");
    }

    const char* filePath = CHAR(STRING_ELT(file_name, 0));
    std::ifstream rFile(filePath);
    if (!rFile.is_open()) {
        Rf_error("Could not open file: %s", filePath);
    }

    std::vector<SEXP> collectedMutations;
    std::string line;
    while (std::getline(rFile, line)) {
        // Trim and skip comments/empty
        line.erase(0, line.find_first_not_of(" \t"));
        if (line.empty() || line[0] == '#') {
            continue;
        }
        line.erase(line.find_last_not_of(" \t") + 1);

        SEXP strVec = PROTECT(Rf_allocVector(STRSXP, 1));
        SET_STRING_ELT(strVec, 0, Rf_mkChar(line.c_str()));
        ParseStatus status;
        SEXP cmdExpr = PROTECT(R_ParseVector(strVec, -1, &status, R_NilValue));
        UNPROTECT(1); // strVec

        if (status != PARSE_OK || Rf_length(cmdExpr) < 1) {
            UNPROTECT(1); // cmdExpr
            continue;
        }

        // For each top-level expression on this line, generate all mutations
        for (int i = 0; i < Rf_length(cmdExpr); i++) {
            SEXP expr = VECTOR_ELT(cmdExpr, i);
            auto mutations = generate_all_mutations(expr);
            collectedMutations.insert(collectedMutations.end(), mutations.begin(), mutations.end());
        }

        UNPROTECT(1); // cmdExpr
    }
    rFile.close();

    // Return as an R list
    SEXP resultList = PROTECT(Rf_allocVector(VECSXP, collectedMutations.size()));
    for (size_t i = 0; i < collectedMutations.size(); i++) {
        SET_VECTOR_ELT(resultList, i, collectedMutations[i]);
    }

    UNPROTECT(1);
    return resultList;
}