#ifndef DEL_OPERATOR_H
#define DEL_OPERATOR_H

#include "Operator.hpp"

class DeleteOperator : public Operator {
public: 
    DeleteOperator(SEXP symbol) : Operator(symbol){}
    virtual ~DeleteOperator() = default;

    std::string getType() const override {
        return "DeleteOperator";
    }

    /* 
        TODO: should not be implemented, against Interface Segregation Principle
        A client should never be forced to implement an interface that it doesn’t 
        use, or clients shouldn’t be forced to depend on methods they do not use.
    */
    void flip(SEXP& node) const override {
        return;
    }
};

#endif