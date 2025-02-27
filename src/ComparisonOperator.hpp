#ifndef COMPARISON_OPERATOR_H
#define COMPARISON_OPERATOR_H

#include "Operator.hpp"

class ComparisonOperator : public Operator {
public:
    ComparisonOperator(SEXP symbol) : Operator(symbol){}
    virtual ~ComparisonOperator() = default;

    virtual std::string getType() const override{
        return "ComparisonOperator";
    }
};

#endif