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
    // Duplicate the list to avoid side effects on the original AST
    SEXP duplicated_list = Rf_duplicate(expr);
    PROTECT(duplicated_list);

    const OperatorPos& op_pos = ops[whichOpIndex];
    const std::vector<int>& path = op_pos.path;
    
    if (path.empty()) {
        UNPROTECT(1);
        return {R_NilValue, false};
    }
    
    // Build mutation message
    std::ostringstream msg;
    msg << "Deleting node at path: ";
    for (size_t i = 0; i < path.size(); i++) {
        msg << path[i];
        if (i < path.size() - 1) msg << "/";
    }
    msg << '\n';
    msg << "From line/col: " << op_pos.start_line << "/" << op_pos.start_col << '\n'
        << "To line/col: " << op_pos.end_line << "/" << op_pos.end_col << '\n';
    
    // Navigate to the parent node containing the element to delete
    SEXP parent = duplicated_list;
    SEXP current = duplicated_list;
    
    // If we're deleting the root expression itself
    if (path.size() == 1 && path[0] == 0) {
        UNPROTECT(1);
        // We can't delete the root expression
        return {duplicated_list, false};
    }
    
    // Navigate to the parent of the node to delete
    for (size_t i = 0; i < path.size() - 1; i++) {
        int idx = path[i];
        SEXP iter = parent;
        for (int j = 0; j < idx; j++) {
            if (CDR(iter) == R_NilValue) {
                UNPROTECT(1);
                return {duplicated_list, false}; // Path is invalid
            }
            iter = CDR(iter);
        }
        parent = CAR(iter);
        
        // Check if we've reached a non-LANGSXP node, which would be an error
        if (TYPEOF(parent) != LANGSXP) {
            UNPROTECT(1);
            return {duplicated_list, false};
        }
    }
    
    // Now parent points to the node containing the element to delete
    int deleteIndex = path.back();
    
    // Special handling for the first element (which is the function/operator symbol)
    if (deleteIndex == 0) {
        // We can't delete the function/operator symbol of a call
        UNPROTECT(1);
        return {duplicated_list, false};
    }
    
    // Navigate to the element just before the one to delete
    SEXP prev = parent;
    for (int i = 0; i < deleteIndex - 1; i++) {
        prev = CDR(prev);
        if (prev == R_NilValue) {
            UNPROTECT(1);
            return {duplicated_list, false}; // Path is invalid
        }
    }
    
    // If the element to delete exists, remove it by skipping it in the list
    if (CDR(prev) != R_NilValue) {
        SETCDR(prev, CDDR(prev));
        
        // Add mutation info
        SEXP msg_sexp = PROTECT(Rf_mkString(msg.str().c_str()));
        Rf_setAttrib(duplicated_list, Rf_install("mutation_info"), msg_sexp);
        UNPROTECT(1); // msg_sexp
        
        UNPROTECT(1); // duplicated_list
        return {duplicated_list, true};
    }
    
    UNPROTECT(1); // duplicated_list
    return {duplicated_list, false};
}

// TODO: extract the message stuff into a function