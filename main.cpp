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

//TODO:
/*
* overwrite the file
* 
*/

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

void printSEXP(SEXP expr) {

    std::cout << "printing" << std::endl;
    Rf_PrintValue(expr);
    std::cout << "finishing printing" << std::endl;
    return;
}

std::string generateNodeKey(SEXP expr) {
    std::ostringstream oss;

    // Add the type of the node
    oss << "TYPEOF: " << TYPEOF(expr);

    // For symbols, add their name
    if (TYPEOF(expr) == SYMSXP) {
        oss << " SYMBOL: " << CHAR(PRINTNAME(expr));
    }
    // For character strings, add their value
    else if (TYPEOF(expr) == STRSXP) {
        oss << " STRING: " << CHAR(STRING_ELT(expr, 0));
    }
    // For language constructs, add the function name
    else if (TYPEOF(expr) == LANGSXP) {
        SEXP fun = CAR(expr);
        if (TYPEOF(fun) == SYMSXP) {
            oss << " FUNC: " << CHAR(PRINTNAME(fun));
        }
    }

    // Add the pointer address as a fallback
    oss << " ADDRESS: " << expr;

    return oss.str();
}

SEXP mutateExpression(SEXP expr, std::vector<MutationReport>& reports, int lineNumber, std::unordered_map<std::string, std::string>& mutationMap) {
    // Generate a unique key for the current node
    std::string nodeKey = generateNodeKey(expr);

    // Check if this node has already been mutated
    if (mutationMap.find(nodeKey) != mutationMap.end()) {
        std::cout << "Skipping already mutated node: " << nodeKey << std::endl;
        return expr;
    }

    if (TYPEOF(expr) == LANGSXP) {
        SEXP fun = CAR(expr); // Get the function part of the language construct

        std::cout << "Printing fun" << std::endl;
        printSEXP(fun);

        std::cout << "Printing expr" << std::endl;
        printSEXP(expr);

        // Apply mutations only once
        if (fun == Rf_install("+") && mutationMap[nodeKey] != "+ to -") {
            std::cout << "Applying mutation: + to -" << std::endl;
            SETCAR(expr, Rf_install("-")); // Mutate "+" to "-"
            reports.push_back({lineNumber, "Original: +", "Mutated: -", "+ to - mutation"});
            mutationMap[nodeKey] = "+ to -"; // Record the mutation
            return expr; // Stop further traversal after mutation
        } else if (fun == Rf_install("-") && mutationMap[nodeKey] != "- to +") {
            std::cout << "Applying mutation: - to +" << std::endl;
            SETCAR(expr, Rf_install("+")); // Mutate "-" to "+"
            reports.push_back({lineNumber, "Original: -", "Mutated: +", "- to + mutation"});
            mutationMap[nodeKey] = "- to +"; // Record the mutation
            return expr; // Stop further traversal after mutation
        }

        // Traverse child nodes recursively
        SEXP next = CDR(expr);
        while (next != R_NilValue) {
            std::cout << "Printing next" << std::endl;
            printSEXP(next);
            mutateExpression(CAR(next), reports, lineNumber, mutationMap);
            next = CDR(next);
        }
    }

    return expr;
}

// Wrapper function to initialize the mutation map
SEXP mutateExpression(SEXP expr, std::vector<MutationReport>& reports, int lineNumber) {
    // Initialize an unordered_map to store mutations
    std::unordered_map<std::string, std::string> mutationMap;
    return mutateExpression(expr, reports, lineNumber, mutationMap);
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

void parseRFileAsString(const std::string& filePath, std::vector<MutationReport>& reports) {
    std::ifstream rFile(filePath);
    if (!rFile.is_open()) {
        std::cerr << "Could not open file: " << filePath << std::endl;
        return;
    }

    std::string line;
    int lineNumber = 0;

    while (std::getline(rFile, line)) {
        lineNumber++;
        std::cout << "Parsing line " << lineNumber << ": " << line << std::endl;

        // Trim leading and trailing whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.empty()) {
            std::cout << "Skipping empty line." << std::endl;
            continue;
        }

        // Parse the single line
        SEXP cmdExpr = R_NilValue;
        PROTECT(cmdExpr = R_ParseString(line.c_str()));

        if (Rf_length(cmdExpr) == 0) {
            std::cerr << "Parse error on line " << lineNumber << ": " << line << std::endl;
            UNPROTECT(1);
            continue; // Skip to the next line on parse failure
        }

        // Process each expression in the parsed line
        for (int i = 0; i < Rf_length(cmdExpr); i++) {
            //SEXP expr = VECTOR_ELT(cmdExpr, i);
            inspectAST(cmdExpr);
            mutateExpression(cmdExpr, reports, lineNumber);
        }

        UNPROTECT(1);
    }

    rFile.close();
}

void runREmbeddedFileParsing(const std::string& filePath) {
    initializeREnvironment();
    std::vector<MutationReport> reports;
    parseRFileAsString(filePath, reports);
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