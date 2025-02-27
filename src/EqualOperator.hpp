#ifndef EQ_OPERATOR_H
#define EQ_OPERATOR_H

#include "ComparisonOperator.hpp"

class EqualOperator: public ComparisonOperator {
public:
    EqualOperator() : ComparisonOperator(Rf_install("==")) {}
    virtual ~EqualOperator() = default;

    std::string getType() const override {
        return "EqualOperator";
    }

    void flip(SEXP& node) const override {
        static SEXP neqSym = Rf_install("!=");
        SETCAR(node, neqSym);
    }
};

#endif