#include <gtest/gtest.h>
#include <R.h>
#include <Rinternals.h>
#include "../src/ASTHandler.hpp"
#include <memory>

// Mock R symbols for testing
class ASTHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize R environment if needed
        // This is required to use Rf_install and other R functions
    }

    void TearDown() override {
        // Clean up R environment if needed
    }

    // Helper method to create a simple R expression
    SEXP createSimpleExpression(const char* op, const char* arg1, const char* arg2) {
        SEXP expr = PROTECT(Rf_allocList(3));
        SETCAR(expr, Rf_install(op));
        SETCAR(CDR(expr), Rf_mkString(arg1));
        SETCAR(CDR(CDR(expr)), Rf_mkString(arg2));
        
        // Create a simple srcref
        SEXP srcref = PROTECT(Rf_allocVector(INTSXP, 4));
        INTEGER(srcref)[0] = 1; // start line
        INTEGER(srcref)[1] = 1; // start col
        INTEGER(srcref)[2] = 1; // end line
        INTEGER(srcref)[3] = 10; // end col
        
        UNPROTECT(2);
        return expr;
    }
};

// Test isDeletable method
TEST_F(ASTHandlerTest, IsDeletable) {
    ASTHandler handler;
    // Create a simple expression
    SEXP expr = PROTECT(Rf_allocList(3));
    SETCAR(expr, Rf_install("+"));
    SETCAR(CDR(expr), Rf_mkString("a"));
    SETCAR(CDR(CDR(expr)), Rf_mkString("b"));
    
    // Test when is_inside_block is false
    SEXP srcref = PROTECT(Rf_allocVector(INTSXP, 4));
    std::vector<OperatorPos> ops = handler.gatherOperators(expr, srcref, false);
    // Verify the expression is not deletable when is_inside_block is false
    // This test assumes isDeletable returns _is_inside_block
    EXPECT_EQ(0, std::count_if(ops.begin(), ops.end(), 
                              [](const OperatorPos& pos) { 
                                  return dynamic_cast<DeleteOperator*>(pos.op.get()) != nullptr; 
                              }));
    
    // Test when is_inside_block is true
    ops = handler.gatherOperators(expr, srcref, true);
    // Verify the expression is deletable when is_inside_block is true
    EXPECT_GT(std::count_if(ops.begin(), ops.end(), 
                           [](const OperatorPos& pos) { 
                               return dynamic_cast<DeleteOperator*>(pos.op.get()) != nullptr; 
                           }), 0);
    
    UNPROTECT(2);
}

// Test gatherOperators method for arithmetic operators
TEST_F(ASTHandlerTest, GatherArithmeticOperators) {
    ASTHandler handler;
    
    // Test for + operator
    SEXP plus_expr = createSimpleExpression("+", "a", "b");
    SEXP srcref = PROTECT(Rf_allocVector(INTSXP, 4));
    INTEGER(srcref)[0] = 1; // start line
    INTEGER(srcref)[1] = 1; // start col
    INTEGER(srcref)[2] = 1; // end line
    INTEGER(srcref)[3] = 10; // end col
    
    std::vector<OperatorPos> ops = handler.gatherOperators(plus_expr, srcref, false);
    
    // Verify we found the + operator
    ASSERT_GT(ops.size(), 0);
    EXPECT_TRUE(dynamic_cast<PlusOperator*>(ops[0].op.get()) != nullptr);
    
    UNPROTECT(1);
    
    // Test for - operator
    SEXP minus_expr = createSimpleExpression("-", "a", "b");
    srcref = PROTECT(Rf_allocVector(INTSXP, 4));
    INTEGER(srcref)[0] = 1;
    INTEGER(srcref)[1] = 1;
    INTEGER(srcref)[2] = 1;
    INTEGER(srcref)[3] = 10;
    
    ops = handler.gatherOperators(minus_expr, srcref, false);
    
    // Verify we found the - operator
    ASSERT_GT(ops.size(), 0);
    EXPECT_TRUE(dynamic_cast<MinusOperator*>(ops[0].op.get()) != nullptr);
    
    UNPROTECT(1);
}

// Test gatherOperators method for comparison operators
TEST_F(ASTHandlerTest, GatherComparisonOperators) {
    ASTHandler handler;
    
    // Test for == operator
    SEXP eq_expr = createSimpleExpression("==", "a", "b");
    SEXP srcref = PROTECT(Rf_allocVector(INTSXP, 4));
    INTEGER(srcref)[0] = 1;
    INTEGER(srcref)[1] = 1;
    INTEGER(srcref)[2] = 1;
    INTEGER(srcref)[3] = 10;
    
    std::vector<OperatorPos> ops = handler.gatherOperators(eq_expr, srcref, false);
    
    // Verify we found the == operator
    ASSERT_GT(ops.size(), 0);
    EXPECT_TRUE(dynamic_cast<EqualOperator*>(ops[0].op.get()) != nullptr);
    
    UNPROTECT(1);
    
    // Test for < operator
    SEXP lt_expr = createSimpleExpression("<", "a", "b");
    srcref = PROTECT(Rf_allocVector(INTSXP, 4));
    INTEGER(srcref)[0] = 1;
    INTEGER(srcref)[1] = 1;
    INTEGER(srcref)[2] = 1;
    INTEGER(srcref)[3] = 10;
    
    ops = handler.gatherOperators(lt_expr, srcref, false);
    
    // Verify we found the < operator
    ASSERT_GT(ops.size(), 0);
    EXPECT_TRUE(dynamic_cast<LessThanOperator*>(ops[0].op.get()) != nullptr);
    
    UNPROTECT(1);
}

// Test gatherOperators method for logical operators
TEST_F(ASTHandlerTest, GatherLogicalOperators) {
    ASTHandler handler;
    
    // Test for && operator
    SEXP and_expr = createSimpleExpression("&&", "a", "b");
    SEXP srcref = PROTECT(Rf_allocVector(INTSXP, 4));
    INTEGER(srcref)[0] = 1;
    INTEGER(srcref)[1] = 1;
    INTEGER(srcref)[2] = 1;
    INTEGER(srcref)[3] = 10;
    
    std::vector<OperatorPos> ops = handler.gatherOperators(and_expr, srcref, false);
    
    // Verify we found the && operator
    ASSERT_GT(ops.size(), 0);
    EXPECT_TRUE(dynamic_cast<LogicalAndOperator*>(ops[0].op.get()) != nullptr);
    
    UNPROTECT(1);
    
    // Test for || operator
    SEXP or_expr = createSimpleExpression("||", "a", "b");
    srcref = PROTECT(Rf_allocVector(INTSXP, 4));
    INTEGER(srcref)[0] = 1;
    INTEGER(srcref)[1] = 1;
    INTEGER(srcref)[2] = 1;
    INTEGER(srcref)[3] = 10;
    
    ops = handler.gatherOperators(or_expr, srcref, false);
    
    // Verify we found the || operator
    ASSERT_GT(ops.size(), 0);
    EXPECT_TRUE(dynamic_cast<LogicalOrOperator*>(ops[0].op.get()) != nullptr);
    
    UNPROTECT(1);
}

// Test path tracking in recursive operator gathering
TEST_F(ASTHandlerTest, PathTracking) {
    ASTHandler handler;
    
    // Create a more complex nested expression: a + (b * c)
    SEXP mul_expr = PROTECT(Rf_allocList(3));
    SETCAR(mul_expr, Rf_install("*"));
    SETCAR(CDR(mul_expr), Rf_mkString("b"));
    SETCAR(CDR(CDR(mul_expr)), Rf_mkString("c"));
    
    SEXP plus_expr = PROTECT(Rf_allocList(3));
    SETCAR(plus_expr, Rf_install("+"));
    SETCAR(CDR(plus_expr), Rf_mkString("a"));
    SETCAR(CDR(CDR(plus_expr)), mul_expr);
    
    SEXP srcref = PROTECT(Rf_allocVector(INTSXP, 4));
    INTEGER(srcref)[0] = 1;
    INTEGER(srcref)[1] = 1;
    INTEGER(srcref)[2] = 1;
    INTEGER(srcref)[3] = 15;
    
    std::vector<OperatorPos> ops = handler.gatherOperators(plus_expr, srcref, false);
    
    // We should have found 2 operators: + and *
    ASSERT_EQ(2, ops.size());
    
    // Check the paths
    // The + operator should be at the root (empty path)
    // The * operator should be at path [1] (second argument of +)
    bool found_plus = false;
    bool found_mul = false;
    
    for (const auto& op : ops) {
        if (dynamic_cast<PlusOperator*>(op.op.get())) {
            found_plus = true;
            EXPECT_EQ(0, op.path.size());
        } else if (dynamic_cast<MultiplyOperator*>(op.op.get())) {
            found_mul = true;
            ASSERT_EQ(1, op.path.size());
            EXPECT_EQ(1, op.path[0]);
        }
    }
    
    EXPECT_TRUE(found_plus);
    EXPECT_TRUE(found_mul);
    
    UNPROTECT(3);
}

// Main function that runs all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
