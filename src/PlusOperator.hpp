// PlusOperator.h
#ifndef PLUS_OPERATOR_H
#define PLUS_OPERATOR_H

#include "ArithmeticOperator.hpp"

class PlusOperator : public ArithmeticOperator {
public:
    PlusOperator() : ArithmeticOperator(Rf_install("+")) {}
    virtual ~PlusOperator() = default;

    std::string getType() const override {
        return "PlusOperator";
    }

    void flip(SEXP& node) const override {
        static SEXP minusSym = Rf_install("-");
        SETCAR(node, minusSym);
    }
};

#endif // PLUS_OPERATOR_H
