#ifndef LT_OPERATOR_H
#define LT_OPERATOR_H

#include "ComparisonOperator.hpp"

class LessThanOperator: public ComparisonOperator {
public:
    LessThanOperator() : ComparisonOperator(Rf_install("<")) {}
    virtual ~LessThanOperator() = default;

    std::string getType() const override {
        return "LessThanOperator";
    }

    void flip(SEXP& node) const override {
        static SEXP moreThanSym = Rf_install(">");
        SETCAR(node, moreThanSym);
    }
};

#endif