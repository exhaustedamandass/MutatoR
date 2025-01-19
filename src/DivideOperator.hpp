#ifndef DIVIDE_OPERATOR_H
#define DIVIDE_OPERATOR_H

#include "ArithmeticOperator.hpp"

class DivideOperator : public ArithmeticOperator {
public:
    DivideOperator() : ArithmeticOperator(Rf_install("/")) {}
    virtual ~DivideOperator() = default;

    std::string getType() const override {
        return "DivideOperator";
    }
};

#endif 