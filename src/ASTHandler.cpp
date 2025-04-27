// ASTHandler.cpp

#include <map>
#include <functional>
#include <iostream>
#include "ASTHandler.hpp"
#include "PlusOperator.hpp"
#include "MinusOperator.hpp"
#include "DivideOperator.hpp"
#include "MultiplyOperator.hpp"
#include "EqualOperator.hpp"
#include "NotEqualOperator.hpp"
#include "LessThanOperator.hpp"
#include "MoreThanOperator.hpp"
#include "LessThanOrEqualOperator.hpp"
#include "MoreThanOrEqualOperator.hpp"
#include "AndOperator.hpp"
#include "OrOperator.hpp"
#include "LogicalOrOperator.hpp"
#include "LogicalAndOperator.hpp"
#include "DeleteOperator.hpp"
        
bool ASTHandler::isDeletable(SEXP expr) {
    return false;
    if (!_is_inside_block) return false;
    
    // We can't delete the block itself, only elements inside the block
    if (TYPEOF(expr) == LANGSXP) {
        SEXP head = CAR(expr);
        if (TYPEOF(head) == SYMSXP) {
            std::string op_name = CHAR(PRINTNAME(head));
            if (op_name == "{" || op_name == "}") {
                return false;
            }
        }
    }
    
    return true;
}

std::vector<OperatorPos> ASTHandler::gatherOperators(SEXP expr, SEXP src_ref, bool is_inside_block) {
    std::vector<OperatorPos> ops;
    std::vector<int> path;
    int* ref_ptr = INTEGER(src_ref);
    _start_line = ref_ptr[0];
    _start_col  = ref_ptr[1];
    _end_line   = ref_ptr[2];
    _end_col    = ref_ptr[3];
    _is_inside_block = is_inside_block;
    gatherOperatorsRecursive(expr, path, ops);
    return ops;
}

void ASTHandler::gatherOperatorsRecursive(SEXP expr, std::vector<int> path, std::vector<OperatorPos>& ops) {
    if (TYPEOF(expr) == LANGSXP) {
        SEXP fun = CAR(expr);

        std::map<SEXP, std::function<std::unique_ptr<Operator>()>> operator_map = {
            {Rf_install("+"), []() { return std::make_unique<PlusOperator>(); }},
            {Rf_install("-"), []() { return std::make_unique<MinusOperator>(); }},
            {Rf_install("*"), []() { return std::make_unique<MultiplyOperator>(); }},
            {Rf_install("/"), []() { return std::make_unique<DivideOperator>(); }},
            {Rf_install("=="), []() { return std::make_unique<EqualOperator>(); }},
            {Rf_install("!="), []() { return std::make_unique<NotEqualOperator>(); }},
            {Rf_install("<"), []() { return std::make_unique<LessThanOperator>(); }},
            {Rf_install(">"), []() { return std::make_unique<MoreThanOperator>(); }},
            {Rf_install("<="), []() { return std::make_unique<LessThanOrEqualOperator>(); }},
            {Rf_install(">="), []() { return std::make_unique<MoreThanOrEqualOperator>(); }},
            {Rf_install("&"), []() { return std::make_unique<AndOperator>(); }},
            {Rf_install("|"), []() { return std::make_unique<OrOperator>(); }},
            {Rf_install("&&"), []() { return std::make_unique<LogicalAndOperator>(); }},
            {Rf_install("||"), []() { return std::make_unique<LogicalOrOperator>(); }}
        };

        auto it = operator_map.find(fun);
        if (it != operator_map.end()) {
            auto op = it->second();
            OperatorPos pos{path, std::move(op), _start_line, 
            _start_col, _end_line, _end_col, fun};
            ops.push_back(std::move(pos));
        }

        bool is_block = false;
        if (TYPEOF(fun) == SYMSXP) {
            std::string op_name = CHAR(PRINTNAME(fun));
            if (op_name == "{") {
                is_block = true;
            }
        }

        // If we're in a deletable context, add delete operator
        if (isDeletable(expr)) {
            auto deleteOp = std::make_unique<DeleteOperator>(expr);
            OperatorPos delPos{path, std::move(deleteOp), _start_line, 
            _start_col, _end_line, _end_col, expr};
            ops.push_back(std::move(delPos));
        }

        // If this is a block, we need to recursively handle its contents
        if (is_block) {
            bool old_is_inside_block = _is_inside_block;
            _is_inside_block = true;  // Set that we're inside a block for recursive calls
            
            // Traverse block contents
            int i = 0;
            for (SEXP next = CDR(expr); next != R_NilValue; next = CDR(next), i++) {
                std::vector<int> child_path = path;
                child_path.push_back(i);
                gatherOperatorsRecursive(CAR(next), child_path, ops);
            }
            
            _is_inside_block = old_is_inside_block;  // Restore previous block state
        } else {
            // Traverse the child expressions for non-block expressions
            int i = 0;
            for (SEXP next = CDR(expr); next != R_NilValue; next = CDR(next), i++) {
                std::vector<int> child_path = path;
                child_path.push_back(i);
                gatherOperatorsRecursive(CAR(next), child_path, ops);
            }
        }
    }
}
