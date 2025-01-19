// MinusOperator.h
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
};

#endif // MINUS_OPERATOR_H
