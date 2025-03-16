// Mutator.h
#ifndef MUTATOR_H
#define MUTATOR_H

#include "OperatorPos.hpp"
#include <R.h>
#include <Rinternals.h>
#include <vector>

// Class to Handle Mutation Application
class Mutator {
public:
    Mutator() = default;
    ~Mutator() = default;

    // Apply a given subset of operator flips to the original expression
    //SEXP applyMutations(SEXP expr, const std::vector<OperatorPos>& ops, int mask);
    std::pair<SEXP, bool> applyMutation(SEXP expr, const std::vector<OperatorPos>& ops,int whichOpIndex);

    std::pair<SEXP, bool> applyFlipMutation(SEXP expr, const std::vector<OperatorPos>& ops,int whichOpIndex);

    std::pair<SEXP, bool> applyDeleteMutation(SEXP expr, const std::vector<OperatorPos>& ops, int whichOpIndex);
};

#endif // MUTATOR_H
