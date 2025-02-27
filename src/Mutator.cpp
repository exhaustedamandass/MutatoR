// Mutator.cpp
#include <sstream>
#include <iostream>  // Needed for std::cout
#include "Mutator.hpp"


SEXP Mutator::applyFlipMutation(SEXP expr,
                                const std::vector<OperatorPos>& ops,
                                int whichOpIndex) 
{
    SEXP mutated = Rf_duplicate(expr);
    PROTECT(mutated);

    const OperatorPos& op_pos = ops[whichOpIndex];

    // Build mutation message
    std::ostringstream msg;
    msg 
    // << "Line " << op_pos.end_line 
    //     << ", Col " << op_pos.end_col 
    //     << ": '" 
        << "'" << CHAR(PRINTNAME(op_pos.original_symbol))
        << "' -> '";
        // << op_pos.op->getType();

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

    // msg << " (flipped operator: " << CHAR(PRINTNAME(node)) << ")";
    // Attach the message as an attribute
    SEXP msg_sexp = Rf_mkString(msg.str().c_str());
    Rf_setAttrib(mutated, Rf_install("mutation_info"), msg_sexp);

    UNPROTECT(1);
    return mutated;
}
