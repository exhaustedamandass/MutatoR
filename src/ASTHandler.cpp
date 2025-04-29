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
        
static struct CachedSyms {
    SEXP s_lbrace  = Rf_install("{");
    SEXP s_rbrace  = Rf_install("}");
    SEXP s_plus    = Rf_install("+");   SEXP s_minus  = Rf_install("-");
    SEXP s_mul     = Rf_install("*");   SEXP s_div    = Rf_install("/");
    SEXP s_eq      = Rf_install("==");  SEXP s_neq    = Rf_install("!=");
    SEXP s_lt      = Rf_install("<");   SEXP s_gt     = Rf_install(">");
    SEXP s_le      = Rf_install("<=");  SEXP s_ge     = Rf_install(">=");
    SEXP s_and     = Rf_install("&");   SEXP s_or     = Rf_install("|");
    SEXP s_land    = Rf_install("&&");  SEXP s_lor    = Rf_install("||");
    SEXP s_srcref  = Rf_install("srcref");
    SEXP s_mutinfo = Rf_install("mutation_info");
} SYM;

bool ASTHandler::isDeletable(SEXP expr)
{
    if (!_is_inside_block) return false;
    if (TYPEOF(expr) != LANGSXP) return true;

    SEXP head = CAR(expr);
    if (TYPEOF(head) == SYMSXP && (head == SYM.s_lbrace || head == SYM.s_rbrace))
        return false;
    return true;
}

std::vector<OperatorPos> ASTHandler::gatherOperators(SEXP expr, SEXP src_ref,
                                                    bool is_inside_block)
{
    if (TYPEOF(src_ref) != INTSXP || LENGTH(src_ref) < 4)
        Rf_error("src_ref must be an integer vector of length 4");

    const int *p = INTEGER(src_ref);
    _start_line = p[0];  _start_col = p[1];
    _end_line   = p[2];  _end_col  = p[3];
    _is_inside_block = is_inside_block;

    std::vector<OperatorPos> ops;
    std::vector<int> path;
    gatherOperatorsRecursive(expr, path, ops);
    return ops;
}

void ASTHandler::gatherOperatorsRecursive(SEXP expr, std::vector<int> path,
                                          std::vector<OperatorPos>& ops)
{
    if (TYPEOF(expr) != LANGSXP)
        return;

    SEXP fun = CAR(expr);

    /* operator map – keys are the cached symbols */
    static const std::map<SEXP, std::function<std::unique_ptr<Operator>()>> op_map = {
        {SYM.s_plus, []{return std::make_unique<PlusOperator>();}},
        {SYM.s_minus, []{return std::make_unique<MinusOperator>();}},
        {SYM.s_mul, []{return std::make_unique<MultiplyOperator>();}},
        {SYM.s_div, []{return std::make_unique<DivideOperator>();}},
        {SYM.s_eq, []{return std::make_unique<EqualOperator>();}},
        {SYM.s_neq, []{return std::make_unique<NotEqualOperator>();}},
        {SYM.s_lt, []{return std::make_unique<LessThanOperator>();}},
        {SYM.s_gt, []{return std::make_unique<MoreThanOperator>();}},
        {SYM.s_le, []{return std::make_unique<LessThanOrEqualOperator>();}},
        {SYM.s_ge, []{return std::make_unique<MoreThanOrEqualOperator>();}},
        {SYM.s_and, []{return std::make_unique<AndOperator>();}},
        {SYM.s_or, []{return std::make_unique<OrOperator>();}},
        {SYM.s_land, []{return std::make_unique<LogicalAndOperator>();}},
        {SYM.s_lor, []{return std::make_unique<LogicalOrOperator>();}}
    };

    if (auto it = op_map.find(fun); it != op_map.end()) {
        auto op = it->second();
        ops.push_back({path, std::move(op), _start_line, _start_col,
                       _end_line, _end_col, fun});
    }

    const bool is_block = (fun == SYM.s_lbrace);

    // add delete operator if allowed
    if (isDeletable(expr)) {
        auto del = std::make_unique<DeleteOperator>(expr);
        ops.push_back({path, std::move(del), _start_line, _start_col,
                       _end_line, _end_col, expr});
    }

    // recurse into children (block or not)
    int idx = 0;
    for (SEXP next = CDR(expr); next != R_NilValue; next = CDR(next), ++idx) {
        auto child_path = path; child_path.push_back(idx);
        gatherOperatorsRecursive(CAR(next), child_path, ops);
    }
}
