// ArithmeticOperator.h
#ifndef ARITHMETIC_OPERATOR_H
#define ARITHMETIC_OPERATOR_H

#include <vector>
#include "Operator.hpp"

class ArithmeticOperator : public Operator {
public:
    ArithmeticOperator(SEXP symbol) : Operator(symbol) {}
    virtual ~ArithmeticOperator() = default;

    void flip(SEXP& node) const override {
        static SEXP plusSym  = Rf_install("+");
        static SEXP minusSym = Rf_install("-");
        static SEXP multSym  = Rf_install("*");
        static SEXP divSym   = Rf_install("/");

        // All arithmetic symbols we consider
        static std::vector<SEXP> all_ops = { plusSym, minusSym, multSym, divSym };

        // Current operator in the AST node
        SEXP current = CAR(node);

        // Find 'current' in 'all_ops'
        auto it = std::find(all_ops.begin(), all_ops.end(), current);

        // If not found (some edge case?), fallback to + or do nothing
        if (it == all_ops.end()) {
            SETCAR(node, plusSym);
            return;
        }

        // We want to pick a different operator from [all_ops].
        // For a simple approach, pick the "next" operator in the vector,
        // wrapping around. 
        // Or you can pick a random one, or systematically generate multiple mutants, etc.
        int current_idx = static_cast<int>(std::distance(all_ops.begin(), it));
        int new_idx = (current_idx + 1) % static_cast<int>(all_ops.size());

        SETCAR(node, all_ops[new_idx]);
    }

    virtual std::string getType() const override {
        return "ArithmeticOperator";
    }
};

#endif // ARITHMETIC_OPERATOR_H