#ifndef OR_OPERATOR_H
#define OR_OPERATOR_H

#include "LogicalOperator.hpp"

class OrOperator: public LogicalOperator {
public:
    OrOperator() : LogicalOperator(Rf_install("|")) {}
    virtual ~OrOperator() = default;

    std::string getType() const override {
        return "OrOperator";
    }

    void flip(SEXP& node) const override {
        static SEXP andSym = Rf_install("&");
        SETCAR(node, andSym);
    }
};

#endif