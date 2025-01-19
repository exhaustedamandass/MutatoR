// ASTHandler.cpp

#include "ASTHandler.hpp"
#include "PlusOperator.hpp"
#include "MinusOperator.hpp"
#include "DivideOperator.hpp"
#include "MultiplyOperator.hpp"

std::vector<OperatorPos> ASTHandler::gatherOperators(SEXP expr) {
    std::vector<OperatorPos> ops;
    std::vector<int> path;
    gatherOperatorsRecursive(expr, path, ops);
    return ops;
}

void ASTHandler::gatherOperatorsRecursive(SEXP expr, std::vector<int> path, std::vector<OperatorPos>& ops) {
    if (TYPEOF(expr) == LANGSXP) {
        SEXP fun = CAR(expr);

        // get srcref
        
        int start_line=-1, start_col=-1, end_line=-1, end_col=-1;
        SEXP src_ref = Rf_getAttrib(expr, Rf_install("srcref"));
        if (src_ref != R_NilValue && TYPEOF(src_ref) == INTSXP && Rf_length(src_ref) >= 4) {
            int* ref_ptr = INTEGER(src_ref);
            start_line = ref_ptr[0];
            start_col  = ref_ptr[1];
            end_line   = ref_ptr[2];
            end_col    = ref_ptr[3];
        }

        //TODO : refactor with a map instead if

        if (fun == Rf_install("+")) {
            auto plus_op = std::make_unique<PlusOperator>();
            OperatorPos pos{path, std::move(plus_op), start_line, 
            start_col, end_line, end_col, Rf_install("+")};
            ops.push_back(std::move(pos));
        }
        else if (fun == Rf_install("-")) {
            auto minus_op = std::make_unique<MinusOperator>();
            OperatorPos pos{path, std::move(minus_op), start_line, 
            start_col, end_line, end_col, Rf_install("-")};
            ops.push_back(std::move(pos));
        }
        else if (fun == Rf_install("*")){
            auto mul_op = std::make_unique<MultiplyOperator>();
            OperatorPos pos{path, std::move(mul_op), start_line, 
            start_col, end_line, end_col, Rf_install("*")};
            ops.push_back(std::move(pos));
        }
        else if(fun == Rf_install("/")){
            auto div_op = std::make_unique<DivideOperator>();
            OperatorPos pos{path, std::move(div_op), start_line, 
            start_col, end_line, end_col, Rf_install("/")};
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
