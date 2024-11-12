#include <Rcpp.h>

using namespace Rcpp;

// Recursive function to traverse and mutate comparison operators in the AST
List mutate_comparisons(List ast_node) {
    for (int i = 0; i < ast_node.size(); i++) {
        // Check if the node is a call (expression)
        if (TYPEOF(ast_node[i]) == LANGSXP) {
            List call = ast_node[i];

            // Check for comparison operators in the AST
            if (call[0] == Rf_install("<") || call[0] == Rf_install(">") ||
                call[0] == Rf_install("<=") || call[0] == Rf_install(">=") ||
                call[0] == Rf_install("==")) {
                
                // List of alternative operators for mutation
                std::vector<Symbol> operators = {
                    Rf_install("<"), Rf_install(">"), Rf_install("<="),
                    Rf_install(">="), Rf_install("==")
                };
                
                // Generate mutated versions
                for (auto& op : operators) {
                    if (op != call[0]) {
                        List mutated_call = clone(call);
                        mutated_call[0] = op; // Change operator
                        ast_node.push_back(mutated_call);  // Add mutation
                    }
                }
            } else {
                // Recurse into child nodes
                ast_node[i] = mutate_comparisons(call);
            }
        }
    }
    return ast_node;
}

// [[Rcpp::export]]
List generate_mutated_asts(List parsed_code) {
    return mutate_comparisons(parsed_code);
}
