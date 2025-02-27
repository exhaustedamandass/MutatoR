#ifndef AND_OPERATOR_H
#define AND_OPERATOR_H

#include "LogicalOperator.hpp"

class AndOperator: public LogicalOperator{
public: 
    AndOperator() : LogicalOperator(Rf_install("&")) {}
    virtual ~AndOperator() = default;

    std::string getType() const override {
        return "AndOperator";
    }

    void flip(SEXP& node) const override {
        static SEXP orSym = Rf_install("|");
        SETCAR(node, orSym);
    }
};

#endif