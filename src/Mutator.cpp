// Mutator.cpp
#include <sstream>
#include <iostream>  // Needed for std::cout
#include "Mutator.hpp"
#include "DeleteOperator.hpp"

std::pair<SEXP,bool> Mutator::applyMutation(SEXP expr, const std::vector<OperatorPos>& ops, int which)
{
    if (which < 0 || which >= static_cast<int>(ops.size()))
        return {R_NilValue, false};

    if (dynamic_cast<DeleteOperator*>(ops[which].op.get()))
        return applyDeleteMutation(expr, ops, which);
    return applyFlipMutation(expr, ops, which);
}

std::pair<SEXP,bool> Mutator::applyFlipMutation(SEXP expr, const std::vector<OperatorPos>& ops, int which)
{
    SEXP mutated = PROTECT(Rf_duplicate(expr));          // [0]

    const OperatorPos &pos = ops[which];
    SEXP node = mutated;
    for (int idx : pos.path) {
        if (node == R_NilValue || CDR(node) == R_NilValue) {
            UNPROTECT(1);
            return {R_NilValue, false};
        }
        SEXP nxt = CDR(node);
        for (int j = 0; j < idx; ++j) {
            nxt = CDR(nxt);
            if (nxt == R_NilValue) {
                UNPROTECT(1);
                return {R_NilValue, false};
            }
        }
        node = CAR(nxt);
    }

    // perform the operator‑specific flip
    pos.op->flip(node);

    // build mutation_info string
    std::ostringstream oss;
    oss << "\nFrom line/col: " << pos.start_line << '/' << pos.start_col << '\n'
        << "To line/col: "   << pos.end_line   << '/' << pos.end_col   << '\n'
        << '\'' << CHAR(PRINTNAME(pos.original_symbol)) << "' -> '"
        << CHAR(PRINTNAME(CAR(node))) << '\'';

    SEXP msg = PROTECT(Rf_mkString(oss.str().c_str())); // [1]
    Rf_setAttrib(mutated, Rf_install("mutation_info"), msg);
    UNPROTECT(1);                                       // drop msg, mutated still protected
    return {mutated, true};
}

std::pair<SEXP,bool> Mutator::applyDeleteMutation(SEXP expr, const std::vector<OperatorPos>& ops, int which)
{
    SEXP dup = PROTECT(Rf_duplicate(expr));             // [0]
    const auto &pos = ops[which];
    const auto &path = pos.path;

    if (path.empty()) { UNPROTECT(1); return {R_NilValue,false}; }
    if (path.size()==1 && path[0]==0) { UNPROTECT(1); return {R_NilValue,false}; }

    // navigate to parent SEXP that owns the element to delete
    SEXP parent = dup;
    for (size_t i = 0; i + 1 < path.size(); ++i) {
        int idx = path[i];
        if (parent == R_NilValue || TYPEOF(parent) != LANGSXP) { UNPROTECT(1); return {R_NilValue,false}; }
        SEXP iter = parent;
        for (int j=0;j<idx;++j) {
            iter = CDR(iter);
            if (iter == R_NilValue) { UNPROTECT(1); return {R_NilValue,false}; }
        }
        parent = CAR(iter);
    }

    int delIdx = path.back();
    if (delIdx == 0) { UNPROTECT(1); return {R_NilValue,false}; }

    // move to the cons cell *before* the one to remove
    SEXP prev = parent;
    for (int i = 0; i < delIdx - 1; ++i) {
        prev = CDR(prev);
        if (prev == R_NilValue) { UNPROTECT(1); return {R_NilValue,false}; }
    }

    if (CDR(prev) != R_NilValue) {
        SETCDR(prev, CDDR(prev));   // skip over the element to delete

        // attach mutation_info
        std::ostringstream oss;
        oss << "Deleting node at path: ";
        for (size_t i=0;i<path.size();++i) { oss<<path[i]; if(i+1<path.size()) oss<<'/'; }
        oss << "\nFrom line/col: " << pos.start_line << '/' << pos.start_col << '\n'
            << "To line/col: "   << pos.end_line   << '/' << pos.end_col   << '\n';
        SEXP msg = PROTECT(Rf_mkString(oss.str().c_str())); // [1]
        Rf_setAttrib(dup, Rf_install("mutation_info"), msg);
        UNPROTECT(1);                                   // drop msg, dup still protected
        return {dup, true};
    }
    UNPROTECT(1);                                       // drop dup – nothing deleted
    return {R_NilValue, false};
}