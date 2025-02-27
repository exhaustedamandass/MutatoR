#ifndef MTEQ_OPERATOR_H
#define MTEQ_OPERATOR_H

#include "ComparisonOperator.hpp"

class MoreThanOrEqualOperator: public ComparisonOperator {
public:
    MoreThanOrEqualOperator() : ComparisonOperator(Rf_install(">=")) {}
    virtual ~MoreThanOrEqualOperator() = default;

    std::string getType() const override {
        return "MoreThanOrEqualOperator";
    }

    void flip(SEXP& node) const override {
        static SEXP lessThanOrEqualSym = Rf_install("<=");
        SETCAR(node, lessThanOrEqualSym);
    }
};

#endif