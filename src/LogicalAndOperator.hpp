#ifndef LOGICAL_AND_OPERATOR_H
#define LOGICAL_AND_OPERATOR_H

#include "LogicalOperator.hpp"

class LogicalAndOperator : public LogicalOperator {
public:
    LogicalAndOperator() : LogicalOperator(Rf_install("&&")) {}
    virtual ~LogicalAndOperator() = default;

    std::string getType() const override {
        return "LogicalAndOperator";
    }

    void flip(SEXP& node) const override {
        static SEXP orSym = Rf_install("||");
        SETCAR(node, orSym);
    }
};

#endif