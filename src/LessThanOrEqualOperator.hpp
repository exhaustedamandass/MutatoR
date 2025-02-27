#ifndef LTEQ_OPERATOR_H
#define LTEQ_OPERATOR_H

#include "ComparisonOperator.hpp"

class LessThanOrEqualOperator: public ComparisonOperator {
public:
    LessThanOrEqualOperator() : ComparisonOperator(Rf_install("<=")) {}
    virtual ~LessThanOrEqualOperator() = default;

    std::string getType() const override {
        return "LessThanOrEqualOperator";
    }

    void flip(SEXP& node) const override {
        static SEXP moreThanOrEqualSym = Rf_install(">=");
        SETCAR(node, moreThanOrEqualSym);
    }
};

#endif