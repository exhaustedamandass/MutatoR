#ifndef MT_OPERATOR_H
#define MT_OPERATOR_H

#include "ComparisonOperator.hpp"

class MoreThanOperator: public ComparisonOperator {
public:
    MoreThanOperator() : ComparisonOperator(Rf_install(">")) {}
    virtual ~MoreThanOperator() = default;

    std::string getType() const override {
        return "MoreThanOperator";
    }

    void flip(SEXP& node) const override {
        static SEXP lessThanSym = Rf_install("<");
        SETCAR(node, lessThanSym);
    }
};

#endif