#ifndef MUL_OPERATOR_H
#define MUL_OPERATOR_H

#include "ArithmeticOperator.hpp"

class MultiplyOperator : public ArithmeticOperator {
public:
    MultiplyOperator() : ArithmeticOperator(Rf_install("*")) {}
    virtual ~MultiplyOperator() = default;

    std::string getType() const override {
        return "MultiplyOperator";
    }

    void flip(SEXP& node) const override {
        static SEXP divSym = Rf_install("/");
        SETCAR(node, divSym);
    }
};

#endif 