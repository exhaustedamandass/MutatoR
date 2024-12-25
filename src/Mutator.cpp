// Mutator.cpp
#include <sstream>
#include "Mutator.hpp"

SEXP Mutator::applySingleMutation(SEXP expr,
                                  const std::vector<OperatorPos>& ops,
                                  int whichOpIndex, 
                                  SEXP newSym) 
{
    SEXP mutated = Rf_duplicate(expr);
    PROTECT(mutated);

    const OperatorPos& op_pos = ops[whichOpIndex];

    // Build mutation message
    std::ostringstream msg;
    msg << "Line " << op_pos.end_line 
        << ", Col " << op_pos.end_col 
        << ": '" 
        << CHAR(PRINTNAME(op_pos.original_symbol))
        << "' -> '"
        << CHAR(PRINTNAME(newSym))
        << "'";

    // Navigate to the operator node
    SEXP node = mutated;
    for (int idx : op_pos.path) {
        SEXP nxt = CDR(node);
        for (int j = 0; j < idx; j++) {
            nxt = CDR(nxt);
        }
        node = CAR(nxt);
    }

    // Actually set the operator symbol
    SETCAR(node, newSym);

    // Attach the message as an attribute
    SEXP msg_sexp = Rf_mkString(msg.str().c_str());
    Rf_setAttrib(mutated, Rf_install("mutation_info"), msg_sexp);

    UNPROTECT(1);
    return mutated;
}
