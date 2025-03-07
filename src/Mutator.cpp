// Mutator.cpp
#include <sstream>
#include <iostream>  // Needed for std::cout
#include "Mutator.hpp"
#include "DeleteOperator.hpp"

SEXP Mutator::applyMutation(SEXP expr, 
                            const std::vector<OperatorPos>& ops,
                            int whichOpIndex)
{
    if (dynamic_cast<DeleteOperator*>(ops[whichOpIndex].op.get()) != nullptr) {
        return applyDeleteMutation(expr, ops, whichOpIndex);
    } 

    return applyFlipMutation(expr, ops, whichOpIndex);
}

SEXP Mutator::applyFlipMutation(SEXP expr,
                                const std::vector<OperatorPos>& ops,
                                int whichOpIndex) 
{
    SEXP mutated = Rf_duplicate(expr);
    PROTECT(mutated);

    const OperatorPos& op_pos = ops[whichOpIndex];

    // Build mutation message
    std::ostringstream msg;
    msg << '\n';
    msg 
    << "From line/col: " << op_pos.start_line << "/" << op_pos.start_col << '\n'
    << "To line/col: " << op_pos.end_line << "/" << op_pos.end_col << '\n'
    << "'" << CHAR(PRINTNAME(op_pos.original_symbol))
        << "' -> '";

    // Navigate to the operator node
    SEXP node = mutated;
    for (int idx : op_pos.path) {
        SEXP nxt = CDR(node);
        for (int j = 0; j < idx; j++) {
            nxt = CDR(nxt);
        }
        node = CAR(nxt);
    }

    // Use the operator-specific flip method to perform the mutation
    op_pos.op->flip(node);
    msg << CHAR(PRINTNAME(CAR(node))) << "'";

    SEXP msg_sexp = Rf_mkString(msg.str().c_str());
    Rf_setAttrib(mutated, Rf_install("mutation_info"), msg_sexp);

    UNPROTECT(1);
    return mutated;
}

// In Mutator.cpp
SEXP Mutator::applyDeleteMutation(SEXP expr, const std::vector<OperatorPos>& ops, int whichOpIndex) {
    // Duplicate the list using Rf_duplicate to avoid side effects on the original AST.
    SEXP duplicated_list = Rf_duplicate(expr);

    // Retrieve the deletion operator's path.
    // For a pairlist, we assume the path contains one element: the index (0-indexed) of the node to delete.
    const std::vector<int>& path = ops[whichOpIndex].path;
    if (path.empty()) {
        // If no path is specified, return the duplicated list unmodified.
        return R_NilValue;
    }
    int deleteIndex = path[0];

    // Special case: if the first node is to be deleted, return its CDR (i.e. the rest of the list).
    if (deleteIndex == 0) {
        SEXP newList = CDR(duplicated_list);
        return newList;
    }

    // Traverse the duplicated pairlist to find the node immediately before the one to delete.
    SEXP current = duplicated_list;
    int currentIndex = 0;
    while (current != R_NilValue && currentIndex < (deleteIndex - 1)) {
        current = CDR(current);
        ++currentIndex;
    }

    // If the traversal fails (e.g., deleteIndex is out of bounds), return the list unchanged.
    if (current == R_NilValue || CDR(current) == R_NilValue) {
        return duplicated_list;
    }

    // Remove the target node by setting the current node's CDR to skip over the deleted node.
    SETCDR(current, CDDR(current));

    // The duplicated_list now has the specified node removed.
    return duplicated_list;
}

