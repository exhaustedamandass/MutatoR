// Mutator.h
#ifndef MUTATOR_H
#define MUTATOR_H

#include "operators/OperatorPos.hpp"
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

    SEXP applySingleMutation(SEXP expr, const std::vector<OperatorPos>& ops,int whichOpIndex, SEXP newSym);
};

#endif // MUTATOR_H
