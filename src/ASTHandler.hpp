// ASTHandler.h
#ifndef AST_HANDLER_H
#define AST_HANDLER_H

#include "OperatorPos.hpp"
#include <R.h>
#include <Rinternals.h>
#include <vector>
#include <memory>

// Class to Handle AST Traversal and Operator Gathering
class ASTHandler {
public:
    ASTHandler() = default;
    ~ASTHandler() = default;

    // Gather all operators in the AST
    std::vector<OperatorPos> gatherOperators(SEXP expr);

private:
    // Recursive helper function
    void gatherOperatorsRecursive(SEXP expr, std::vector<int> path, std::vector<OperatorPos>& ops);
};

#endif // AST_HANDLER_H