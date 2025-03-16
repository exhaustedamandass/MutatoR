// Mutator.cpp
#include <sstream>
#include <iostream>  // Needed for std::cout
#include "Mutator.hpp"
#include "DeleteOperator.hpp"

std::pair<SEXP, bool> Mutator::applyMutation(SEXP expr, 
                            const std::vector<OperatorPos>& ops,
                            int whichOpIndex)
{
    if (dynamic_cast<DeleteOperator*>(ops[whichOpIndex].op.get()) != nullptr) {
        return applyDeleteMutation(expr, ops, whichOpIndex);
    } 

    return applyFlipMutation(expr, ops, whichOpIndex);
}

std::pair<SEXP, bool> Mutator::applyFlipMutation(SEXP expr,
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
    return {mutated, true};
}

// In Mutator.cpp
std::pair<SEXP, bool> Mutator::applyDeleteMutation(SEXP expr, const std::vector<OperatorPos>& ops, int whichOpIndex) {
    // Duplicate the list using Rf_duplicate to avoid side effects on the original AST.
    SEXP duplicated_list = Rf_duplicate(expr);

    const OperatorPos& op_pos = ops[whichOpIndex];
    // Retrieve the deletion operator's path.
    const std::vector<int>& path = op_pos.path;
    if (path.empty()) {
        // If no path is specified, return an empty SEXP (R's NULL)
        return {R_NilValue, false};
    }
    int deleteIndex = path[0];
    
    // Build mutation message
    std::ostringstream msg;
    msg << "Deleting node at index: " << deleteIndex << '\n';
    msg 
    << "From line/col: " << op_pos.start_line << "/" << op_pos.start_col << '\n'
    << "To line/col: " << op_pos.end_line << "/" << op_pos.end_col << '\n';
    // Special case: if the first node is to be deleted, return its CDR (i.e. the rest of the list).
    // to avoid deleting the 0 index of an expression
    if (deleteIndex == 0) {
        SEXP newList = CDR(duplicated_list);
        
        // Assign mutation info
        SEXP msg_sexp = Rf_mkString(msg.str().c_str());
        Rf_setAttrib(newList, Rf_install("mutation_info"), msg_sexp);
        
        return {newList, false};
    }

    // Traverse the duplicated pairlist to find the node immediately before the one to delete.
    SEXP current = duplicated_list;
    int currentIndex = 0;
    while (current != R_NilValue && currentIndex < (deleteIndex - 1)) {
        current = CDR(current);
        ++currentIndex;
    }

    // If the traversal fails (e.g., deleteIndex is out of bounds), return the list unchanged.
    // don't add it to the actual mutated expression
    if (current == R_NilValue || CDR(current) == R_NilValue) {
        return {duplicated_list, false};
    }

    // Remove the target node by setting the current node's CDR to skip over the deleted node.
    SETCDR(current, CDDR(current));

    // Assign mutation info to the mutated list
    SEXP msg_sexp = Rf_mkString(msg.str().c_str());
    Rf_setAttrib(duplicated_list, Rf_install("mutation_info"), msg_sexp);

    // The duplicated_list now has the specified node removed.
    return {duplicated_list, true};
}

// TODO: extract the message stuff into a function