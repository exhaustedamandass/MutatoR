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
        // SEXP src_ref = Rf_getAttrib(expr, Rf_install("srcref"));
        // // if (src_ref != R_NilValue) {
        // //     std::cout << "src_ref is not null " << TYPEOF(src_ref) <<Rf_length(src_ref) << std::endl;
        // // }
        // if (src_ref != R_NilValue && TYPEOF(src_ref) == INTSXP && Rf_length(src_ref) >= 4) {            
        //     int* ref_ptr = INTEGER(src_ref);
        //     start_line = ref_ptr[0];
        //     start_col  = ref_ptr[1];
        //     end_line   = ref_ptr[2];
        //     end_col    = ref_ptr[3];
        // }
std::vector<OperatorPos> ASTHandler::gatherOperators(SEXP expr) {
    std::vector<OperatorPos> ops;
    std::vector<int> path;
    gatherOperatorsRecursive(expr, path, ops);
    return ops;
}

void ASTHandler::gatherOperatorsRecursive(SEXP expr, std::vector<int> path, std::vector<OperatorPos>& ops) {
    if (TYPEOF(expr) == LANGSXP) {
        SEXP fun = CAR(expr);
        std::cout << "fun now is" << std::endl;
        Rf_PrintValue(fun);
        std::cout << "expr now is" << std::endl;
        Rf_PrintValue(expr);
        SEXP src_ref_cur = Rf_getAttrib(expr, Rf_install("srcref"));
        std::cout << Rf_length(src_ref_cur) << std::endl;
        // get srcref
        
        int start_line=-1, start_col=-1, end_line=-1, end_col=-1;

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
            {Rf_install("|"), []() { return std::make_unique<OrOperator>(); }}
        };

        auto it = operator_map.find(fun);
        if (it != operator_map.end()) {
            auto op = it->second();
            OperatorPos pos{path, std::move(op), start_line, 
            start_col, end_line, end_col, fun};
            ops.push_back(std::move(pos));
        }

        // Traverse the child expressions
        int i = 0;
        for (SEXP next = CDR(expr); next != R_NilValue; next = CDR(next), i++) {
            std::vector<int> child_path = path;
            child_path.push_back(i);
            gatherOperatorsRecursive(CAR(next), child_path, ops);
        }
    }
}
