#include <gtest/gtest.h>
#include <R.h>
#include <Rinternals.h>
#include "../src/Mutator.hpp"
#include "../src/PlusOperator.hpp"
#include "../src/MinusOperator.hpp"
#include "../src/DeleteOperator.hpp"
#include <memory>
#include <vector>

class MutatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test objects
        mutator = std::make_unique<Mutator>();
    }

    // Helper method to create a simple R expression: a + b
    SEXP createPlusExpression() {
        SEXP expr = PROTECT(Rf_allocList(3));
        SETCAR(expr, Rf_install("+"));
        SETCAR(CDR(expr), Rf_mkString("a"));
        SETCAR(CDR(CDR(expr)), Rf_mkString("b"));
        UNPROTECT(1);
        return expr;
    }

    // Helper method to create a plus operator position
    OperatorPos createPlusOperatorPos() {
        std::vector<int> path; // Empty path for root operator
        auto op = std::make_unique<PlusOperator>();
        SEXP plus_symbol = Rf_install("+");
        return OperatorPos(path, std::move(op), 1, 1, 1, 5, plus_symbol);
    }

    // Helper method to create a binary operation
    SEXP createBinaryOperation(const char* op, const char* arg1, const char* arg2) {
        SEXP expr = PROTECT(Rf_allocList(3));
        SETCAR(expr, Rf_install(op));
        SETCAR(CDR(expr), Rf_mkString(arg1));
        SETCAR(CDR(CDR(expr)), Rf_mkString(arg2));
        UNPROTECT(1);
        return expr;
    }

    // Helper method to create a nested expression: a + (b * c)
    SEXP createNestedExpression() {
        SEXP mul_expr = PROTECT(Rf_allocList(3));
        SETCAR(mul_expr, Rf_install("*"));
        SETCAR(CDR(mul_expr), Rf_mkString("b"));
        SETCAR(CDR(CDR(mul_expr)), Rf_mkString("c"));
        
        SEXP plus_expr = PROTECT(Rf_allocList(3));
        SETCAR(plus_expr, Rf_install("+"));
        SETCAR(CDR(plus_expr), Rf_mkString("a"));
        SETCAR(CDR(CDR(plus_expr)), mul_expr);
        
        UNPROTECT(2);
        return plus_expr;
    }

    std::unique_ptr<Mutator> mutator;
};

// Test applyMutation with a simple operator flip
TEST_F(MutatorTest, ApplyMutationFlip) {
    SEXP expr = createPlusExpression();
    PROTECT(expr);
    
    // Create operator positions list with one PlusOperator
    std::vector<OperatorPos> ops;
    ops.push_back(createPlusOperatorPos());
    
    // Apply mutation at index 0
    auto result = mutator->applyMutation(expr, ops, 0);
    
    // Check result
    EXPECT_TRUE(result.second); // Mutation applied successfully
    EXPECT_NE(result.first, expr); // Result is different from original
    
    // Check if the mutation changed + to -
    SEXP mutated = result.first;
    EXPECT_STREQ(CHAR(PRINTNAME(CAR(mutated))), "-");
    
    // Check attributes for mutation info
    SEXP mutation_info = Rf_getAttrib(mutated, Rf_install("mutation_info"));
    EXPECT_FALSE(Rf_isNull(mutation_info));
    
    UNPROTECT(1);
}

// Test applyDeleteMutation with a node deletion
TEST_F(MutatorTest, ApplyDeleteMutation) {
    // Create a simple expression with 3 elements
    SEXP expr = createBinaryOperation("+", "a", "b");
    PROTECT(expr);
    
    // Create operator positions list with one DeleteOperator targeting the first argument
    std::vector<OperatorPos> ops;
    std::vector<int> path = {1}; // Target the first argument (index 1)
    auto op = std::make_unique<DeleteOperator>(CDR(expr));
    SEXP orig_symbol = CDR(expr);
    ops.push_back(OperatorPos(path, std::move(op), 1, 1, 1, 5, orig_symbol));
    
    // Apply deletion mutation
    auto result = mutator->applyDeleteMutation(expr, ops, 0);
    
    // Check result
    EXPECT_TRUE(result.second); // Mutation applied successfully
    EXPECT_NE(result.first, expr); // Result is different from original
    
    // Check if the node was deleted (the list should be shorter)
    SEXP mutated = result.first;
    EXPECT_STREQ(CHAR(PRINTNAME(CAR(mutated))), "+");
    // The first argument should now be "b" because "a" was deleted
    EXPECT_STREQ(CHAR(PRINTNAME(CADR(mutated))), "b");
    
    // Check attributes for mutation info
    SEXP mutation_info = Rf_getAttrib(mutated, Rf_install("mutation_info"));
    EXPECT_FALSE(Rf_isNull(mutation_info));
    
    UNPROTECT(1);
}

// Test deleting the first element in the expression
TEST_F(MutatorTest, ApplyDeleteFirstElement) {
    // Create a simple expression
    SEXP expr = createBinaryOperation("+", "a", "b");
    PROTECT(expr);
    
    // Create operator positions list with one DeleteOperator targeting the operator itself (index 0)
    std::vector<OperatorPos> ops;
    std::vector<int> path = {0}; // Target the operator (index 0)
    auto op = std::make_unique<DeleteOperator>(expr);
    SEXP orig_symbol = CAR(expr);
    ops.push_back(OperatorPos(path, std::move(op), 1, 1, 1, 5, orig_symbol));
    
    // Apply deletion mutation
    auto result = mutator->applyDeleteMutation(expr, ops, 0);
    
    // Check result
    EXPECT_FALSE(result.second); // Special case handling
    EXPECT_NE(result.first, expr); // Result is different from original
    
    // The new list should start with the former CDR of the original list
    SEXP mutated = result.first;
    EXPECT_STREQ(CHAR(PRINTNAME(CAR(mutated))), "a");
    
    // Check attributes for mutation info
    SEXP mutation_info = Rf_getAttrib(mutated, Rf_install("mutation_info"));
    EXPECT_FALSE(Rf_isNull(mutation_info));
    
    UNPROTECT(1);
}

// Test with index out of bounds
TEST_F(MutatorTest, ApplyDeleteOutOfBounds) {
    SEXP expr = createBinaryOperation("+", "a", "b");
    PROTECT(expr);
    
    // Create operator positions list with one DeleteOperator with out-of-bounds path
    std::vector<OperatorPos> ops;
    std::vector<int> path = {5}; // Index that doesn't exist
    auto op = std::make_unique<DeleteOperator>(expr);
    SEXP orig_symbol = CAR(expr);
    ops.push_back(OperatorPos(path, std::move(op), 1, 1, 1, 5, orig_symbol));
    
    // Apply deletion mutation
    auto result = mutator->applyDeleteMutation(expr, ops, 0);
    
    // Check result - should return original expression with mutation flag false
    EXPECT_FALSE(result.second); // Mutation not applied
    EXPECT_EQ(TYPEOF(result.first), TYPEOF(expr)); // Result should be same type as original
    
    UNPROTECT(1);
}

// Test with a nested expression
TEST_F(MutatorTest, ApplyMutationNestedExpression) {
    SEXP expr = createNestedExpression();
    PROTECT(expr);
    
    // Create operator positions list with one operator targeting the nested multiplication
    std::vector<OperatorPos> ops;
    std::vector<int> path = {1}; // Path to the multiply operator
    auto op = std::make_unique<MinusOperator>();
    SEXP mul_symbol = Rf_install("*");
    ops.push_back(OperatorPos(path, std::move(op), 1, 5, 1, 10, mul_symbol));
    
    // Apply mutation to the nested multiply
    auto result = mutator->applyFlipMutation(expr, ops, 0);
    
    // Check result
    EXPECT_TRUE(result.second); // Mutation applied successfully
    EXPECT_NE(result.first, expr); // Result is different from original
    
    // Check if the nested operation was mutated from * to -
    SEXP mutated = result.first;
    SEXP nested_op = CAR(CADDR(mutated));
    EXPECT_STREQ(CHAR(PRINTNAME(nested_op)), "-");
    
    // The rest of the structure should remain the same
    EXPECT_STREQ(CHAR(PRINTNAME(CAR(mutated))), "+");
    
    // Check attributes for mutation info
    SEXP mutation_info = Rf_getAttrib(mutated, Rf_install("mutation_info"));
    EXPECT_FALSE(Rf_isNull(mutation_info));
    
    UNPROTECT(1);
}

// Test with different operator types
TEST_F(MutatorTest, DifferentOperatorTypes) {
    // Test with different binary operations
    const char* operators[] = {"+", "-", "*", "/", "==", "!=", "<", ">", "<=", ">=", "&&", "||"};
    
    for (const char* op : operators) {
        SEXP expr = createBinaryOperation(op, "a", "b");
        PROTECT(expr);
        
        // Create operator positions list with a DeleteOperator for this expression
        std::vector<OperatorPos> ops;
        std::vector<int> path = {1}; // Target first argument
        auto del_op = std::make_unique<DeleteOperator>(CDR(expr));
        SEXP orig_symbol = CAR(expr);
        ops.push_back(OperatorPos(path, std::move(del_op), 1, 1, 1, 5, orig_symbol));
        
        // Apply deletion
        auto result = mutator->applyDeleteMutation(expr, ops, 0);
        
        // Check result
        EXPECT_TRUE(result.second); // Mutation should succeed for all operator types
        EXPECT_NE(result.first, expr); // Result should be different from original
        
        UNPROTECT(1);
    }
}

// Main function that runs all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
