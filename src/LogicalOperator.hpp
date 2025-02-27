#ifndef LOGICAL_OPERATOR_H
#define LOGICAL_OPERATOR_H

#include "Operator.hpp"

class LogicalOperator : public Operator {
public: 
    LogicalOperator(SEXP symbol) : Operator(symbol){}
    virtual ~LogicalOperator() = default;

    virtual std::string getType() const override{
        return "LogicalOperator";
    }
};

#endif