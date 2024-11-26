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


// Function to mutate an R expression (AST)
SEXP mutate_ast(SEXP expr) {
    if (TYPEOF(expr) == LANGSXP) {
        SEXP fun = CAR(expr); // The function/operator of the expression

        // Mutate "+" to "-" and "-" to "+"
        if (fun == Rf_install("+")) {
            SETCAR(expr, Rf_install("-"));
        } else if (fun == Rf_install("-")) {
            SETCAR(expr, Rf_install("+"));
        }

        // Recursively mutate child nodes
        SEXP next = CDR(expr);
        while (next != R_NilValue) {
            mutate_ast(CAR(next));
            next = CDR(next);
        }
    }
    return expr;
}

// C++ function to read and mutate an R file
extern "C" SEXP C_mutate(SEXP file_name) {
    const char* filePath = CHAR(STRING_ELT(file_name, 0));

    // Open the file
    std::ifstream rFile(filePath);
    if (!rFile.is_open()) {
        Rf_error("Could not open file: %s", filePath);
        return R_NilValue;
    }

    std::string line;
    std::ostringstream mutatedContent;

    while (std::getline(rFile, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.empty()) {
            mutatedContent << "\n";
            continue;
        }

        // Parse the line into an expression
        SEXP parsedExpr;
        PROTECT(parsedExpr = R_ParseString(line.c_str()));

        if (Rf_length(parsedExpr) == 0) {
            Rf_warning("Parse error on line: %s", line.c_str());
            mutatedContent << line << "\n"; // Keep original line if parsing fails
            UNPROTECT(1);
            continue;
        }

        // Mutate each expression in the parsed line
        for (int i = 0; i < Rf_length(parsedExpr); i++) {
            // SEXP expr = VECTOR_ELT(parsedExpr, i);
            mutate_ast(parsedExpr);

            // Convert the mutated expression back to a string
            SEXP deparsedExpr = R_NilValue;
            PROTECT(deparsedExpr = Rf_lang2(Rf_install("deparse"), parsedExpr));
            SEXP deparsedResult = R_tryEval(deparsedExpr, R_GlobalEnv, nullptr);

            if (deparsedResult != R_NilValue) {
                mutatedContent << CHAR(STRING_ELT(deparsedResult, 0)) << "\n";
            } else {
                Rf_warning("Failed to deparse mutated expression on line: %s", line.c_str());
                mutatedContent << line << "\n";
            }

            UNPROTECT(1); // Unprotect deparsedExpr
        }

        UNPROTECT(1); // Unprotect parsedExpr
    }

    rFile.close();

    // Return the mutated content as a single string
    return Rf_mkString(mutatedContent.str().c_str());
}