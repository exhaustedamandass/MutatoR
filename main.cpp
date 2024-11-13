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



// Structure to store mutation details
struct MutationReport {
    int lineNumber;
    std::string originalExpr;
    std::string mutatedExpr;
    std::string description;
};

// Function to initialize the R environment
void initializeREnvironment() {
    int argc = 2;
    const char* argv[] = {"R", "--no-save"};
    Rf_initEmbeddedR(argc, (char **)argv);
}

// Function to mutate specific nodes in the AST
SEXP mutateExpression(SEXP expr, std::vector<MutationReport>& reports, int lineNumber, std::unordered_set<SEXP>& visited) {
    // Check if node has been visited
    if (visited.find(expr) != visited.end()) {
        std::cout << "Skipping already visited node." << std::endl;
        return expr;
    }
    visited.insert(expr);

    std::cout << "Visiting new node, type: " << TYPEOF(expr) << std::endl;

    if (TYPEOF(expr) == LANGSXP) {  // Check if it's a language construct
        SEXP fun = CAR(expr);

        // Mutation: Replace "+" with "-"
        if (fun == Rf_install("+")) {
            std::cout << "Applying mutation: + to -" << std::endl;
            SETCAR(expr, Rf_install("-"));
            reports.push_back({lineNumber, "Original: +", "Mutated: -", "+ to - mutation"});
        } 
        // Mutation: Replace "-" with "+"
        else if (fun == Rf_install("-")) {
            std::cout << "Applying mutation: - to +" << std::endl;
            SETCAR(expr, Rf_install("+"));
            reports.push_back({lineNumber, "Original: -", "Mutated: +", "- to + mutation"});
        } 
        // Mutation: Replace "*" with "/"
        else if (fun == Rf_install("*")) {
            std::cout << "Applying mutation: * to /" << std::endl;
            SETCAR(expr, Rf_install("/"));
            reports.push_back({lineNumber, "Original: *", "Mutated: /", "* to / mutation"});
        }
        // Mutation: Replace ">" with "<"
        else if (fun == Rf_install(">")) {
            std::cout << "Applying mutation: > to <" << std::endl;
            SETCAR(expr, Rf_install("<"));
            reports.push_back({lineNumber, "Original: >", "Mutated: <", "> to < mutation"});
        }
    }

    // Recursively apply mutations to child nodes
    for (SEXP next = CDR(expr); next != R_NilValue; next = CDR(next)) {
        std::cout << "Recursively visiting child node." << std::endl;
        mutateExpression(CAR(next), reports, lineNumber, visited);
    }

    std::cout << "Returning from node, type: " << TYPEOF(expr) << std::endl;
    return expr;
}

// Wrapper function to initialize the visited set
SEXP mutateExpression(SEXP expr, std::vector<MutationReport>& reports, int lineNumber) {
    std::unordered_set<SEXP> visited;
    return mutateExpression(expr, reports, lineNumber, visited);
}

// Function to print the AST structure using .Internal(inspect())
void inspectAST(SEXP expr) {
    int errorOccurred = 0;
    SEXP inspectCall = Rf_lang2(Rf_install(".Internal"), Rf_lang2(Rf_install("inspect"), expr));
    R_tryEval(inspectCall, R_GlobalEnv, &errorOccurred);

    if (errorOccurred) {
        std::cerr << "Error occurred during .Internal(inspect())" << std::endl;
    }
}

void parseRFileAsBlock(const std::string &filePath, std::vector<MutationReport>& reports) {
    std::ifstream rFile(filePath);
    if (!rFile.is_open()) {
        std::cerr << "Could not open file: " << filePath << std::endl;
        return;
    }

    std::stringstream buffer;
    buffer << rFile.rdbuf();
    std::string fileContent = buffer.str();

    SEXP cmdSexp, cmdExpr = R_NilValue;
    ParseStatus status;

    PROTECT(cmdSexp = Rf_allocVector(STRSXP, 1));
    SET_STRING_ELT(cmdSexp, 0, Rf_mkChar(fileContent.c_str()));

    PROTECT(cmdExpr = R_ParseVector(cmdSexp, -1, &status, R_NilValue));

    if (status != PARSE_OK) {
        std::cerr << "Error parsing file content" << std::endl;
    } else {
        for (int i = 0; i < Rf_length(cmdExpr); i++) {
            int lineNumber = i + 1;
            std::cout << "\nInspecting Expression " << lineNumber << " with .Internal(inspect()):" << std::endl;
            inspectAST(VECTOR_ELT(cmdExpr, i));
            mutateExpression(VECTOR_ELT(cmdExpr, i), reports, lineNumber);
        }
    }

    UNPROTECT(2);
    rFile.close();
}

void runREmbeddedFileParsing(const std::string &filePath) {
    initializeREnvironment();
    std::vector<MutationReport> reports;
    parseRFileAsBlock(filePath, reports);
    Rf_endEmbeddedR(0);

    // Display mutation reports
    for (const auto& report : reports) {
        std::cout << "Line " << report.lineNumber << ": " << report.description << "\n";
        std::cout << "Original Expression: " << report.originalExpr << "\n";
        std::cout << "Mutated Expression: " << report.mutatedExpr << "\n\n";
    }
}

int main() {
    const std::string filePath = "sample/sample.R";
    runREmbeddedFileParsing(filePath);
    return 0;
}