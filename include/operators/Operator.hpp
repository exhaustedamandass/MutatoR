// Operator.h
#ifndef OPERATOR_H
#define OPERATOR_H

// Abstract Base Class
#include <cpp11.hpp>
#include <R.h>
#include <Rinternals.h>
#include <Rembedded.h>
#include <R_ext/Parse.h>

// Undefine the 'length' macro defined by Rinternals.h to avoid conflicts with the C++ standard library
#undef length

class Operator {
public:
    Operator(SEXP symbol) : operator_symbol(symbol) {}
    virtual ~Operator() = default;

    // Return the symbol of the operator (e.g., "+" or "-")
    SEXP getSymbol() const { return operator_symbol; }

    // Method to flip the operator; pure virtual to enforce implementation in derived classes
    virtual void flip(SEXP& node) const = 0;

    // Optionally, return the name/type of the operator
    virtual std::string getType() const = 0;

protected:
    SEXP operator_symbol;
};

#endif // OPERATOR_H