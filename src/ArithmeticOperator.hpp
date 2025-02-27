#ifndef ARITHMETIC_OPERATOR_H
#define ARITHMETIC_OPERATOR_H

#include <vector>
#include "Operator.hpp"

class ArithmeticOperator : public Operator {
public:
    ArithmeticOperator(SEXP symbol) : Operator(symbol) {}
    virtual ~ArithmeticOperator() = default;

    virtual std::string getType() const override {
        return "ArithmeticOperator";
    }
};

#endif