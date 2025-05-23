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
    std::vector<OperatorPos> gatherOperators(SEXP expr, SEXP src_ref, bool is_inside_block);

private:
    int _start_line;
    int _start_col;
    int _end_line;
    int _end_col;
    bool _is_inside_block;
    // Recursive helper function
    void gatherOperatorsRecursive(SEXP expr, std::vector<int> path, std::vector<OperatorPos>& ops);

    bool isDeletable(SEXP expr);
};

#endif // AST_HANDLER_H