#ifndef NEQ_OPERATOR_H
#define NEQ_OPERATOR_H

#include "ComparisonOperator.hpp"

class NotEqualOperator: public ComparisonOperator {
public:
    NotEqualOperator() : ComparisonOperator(Rf_install("!=")) {}
    virtual ~NotEqualOperator() = default;

    std::string getType() const override {
        return "NotEqualOperator";
    }

    void flip(SEXP& node) const override {
        static SEXP eqSym = Rf_install("==");
        SETCAR(node, eqSym);
    }
};

#endif