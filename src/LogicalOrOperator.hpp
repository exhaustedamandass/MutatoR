// LogicalOrOperator.hpp
#ifndef LOGICAL_OR_OPERATOR_H
#define LOGICAL_OR_OPERATOR_H

#include "LogicalOperator.hpp"

class LogicalOrOperator : public LogicalOperator {
public:
    LogicalOrOperator() : LogicalOperator(Rf_install("||")) {}
    virtual ~LogicalOrOperator() = default;

    std::string getType() const override {
        return "LogicalOrOperator";
    }

    void flip(SEXP& node) const override {
        static SEXP andSym = Rf_install("&&");
        SETCAR(node, andSym);
    }
};

#endif