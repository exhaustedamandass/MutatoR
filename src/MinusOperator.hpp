#ifndef MINUS_OPERATOR_H
#define MINUS_OPERATOR_H

#include "ArithmeticOperator.hpp"

class MinusOperator : public ArithmeticOperator {
public:
    MinusOperator() : ArithmeticOperator(Rf_install("-")) {}
    virtual ~MinusOperator() = default;

    std::string getType() const override {
        return "MinusOperator";
    }

    void flip(SEXP& node) const override {
        static SEXP plusSym = Rf_install("+");
        SETCAR(node, plusSym);
    }
};

#endif // MINUS_OPERATOR_H
