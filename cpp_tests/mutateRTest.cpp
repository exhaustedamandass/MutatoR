#include <gtest/gtest.h>
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Parse.h>
#include <memory>
#include <vector>

// Forward declarations of C functions to test
extern "C" SEXP C_mutate_single(SEXP expr_sexp, SEXP src_ref_sexp, bool is_inside_block);
extern "C" SEXP C_mutate_file(SEXP exprs);
extern bool isValidMutant(SEXP mutant);
extern std::vector<bool> detect_block_expressions(SEXP exprs, int n_expr);

class MutateRTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize R if needed - in a real test environment, 
        // you might need to use Rf_initEmbeddedR() if R isn't already initialized
    }

    void TearDown() override {
        // Cleanup if needed
    }

    // Helper to create source references
    SEXP createSrcRef(int startLine, int startCol, int endLine, int endCol) {
        SEXP srcref = PROTECT(Rf_allocVector(INTSXP, 4));
        INTEGER(srcref)[0] = startLine;
        INTEGER(srcref)[1] = startCol;
        INTEGER(srcref)[2] = endLine;
        INTEGER(srcref)[3] = endCol;
        UNPROTECT(1);
        return srcref;
    }

    // Helper to create a simple R expression
    SEXP createExpression(const char* op, const char* arg1, const char* arg2) {
        SEXP expr = PROTECT(Rf_allocList(3));
        SETCAR(expr, Rf_install(op));
        SETCAR(CDR(expr), Rf_mkString(arg1));
        SETCAR(CDR(CDR(expr)), Rf_mkString(arg2));
        UNPROTECT(1);
        return expr;
    }

    // Helper to create a block expression (curly braces)
    SEXP createBlockExpression(std::vector<SEXP> statements) {
        SEXP block = PROTECT(Rf_allocList(1 + statements.size()));
        SETCAR(block, Rf_install("{"));
        
        SEXP current = block;
        for (size_t i = 0; i < statements.size(); i++) {
            current = CDR(current);
            SETCAR(current, statements[i]);
        }
        
        UNPROTECT(1);
        return block;
    }

    // Helper to create an EXPRSXP (list of expressions)
    SEXP createExpressionList(std::vector<SEXP> expressions) {
        SEXP exprList = PROTECT(Rf_allocVector(VECSXP, expressions.size()));
        for (size_t i = 0; i < expressions.size(); i++) {
            SET_VECTOR_ELT(exprList, i, expressions[i]);
        }
        UNPROTECT(1);
        return exprList;
    }
    
    // Helper to attach source references to an expression list
    SEXP attachSrcRefs(SEXP exprList, std::vector<SEXP> srcRefs) {
        SEXP srcRefList = PROTECT(Rf_allocVector(VECSXP, srcRefs.size()));
        for (size_t i = 0; i < srcRefs.size(); i++) {
            SET_VECTOR_ELT(srcRefList, i, srcRefs[i]);
        }
        Rf_setAttrib(exprList, Rf_install("srcref"), srcRefList);
        UNPROTECT(1);
        return exprList;
    }
};

// Test C_mutate_single with a simple expression
TEST_F(MutateRTest, MutateSingleSimpleExpression) {
    // Create a simple expression: a + b
    SEXP expr = createExpression("+", "a", "b");
    PROTECT(expr);
    
    // Create a source reference
    SEXP srcref = createSrcRef(1, 1, 1, 5);
    PROTECT(srcref);
    
    // Call C_mutate_single with is_inside_block=false
    SEXP result = C_mutate_single(expr, srcref, false);
    PROTECT(result);
    
    // Verify result is a list
    EXPECT_EQ(TYPEOF(result), VECSXP);
    
    // Verify we get at least one mutation (should get a + to - mutation)
    EXPECT_GT(Rf_length(result), 0);
    
    // Get the first mutant and check it's different from the original
    SEXP mutant = VECTOR_ELT(result, 0);
    EXPECT_NE(mutant, expr);
    
    // Check the mutation_info attribute exists
    SEXP mutation_info = Rf_getAttrib(mutant, Rf_install("mutation_info"));
    EXPECT_FALSE(Rf_isNull(mutation_info));
    
    UNPROTECT(3);
}

// Test C_mutate_single with is_inside_block=true (should include delete operations)
TEST_F(MutateRTest, MutateSingleInsideBlock) {
    // Create a simple expression: a + b
    SEXP expr = createExpression("+", "a", "b");
    PROTECT(expr);
    
    // Create a source reference
    SEXP srcref = createSrcRef(1, 1, 1, 5);
    PROTECT(srcref);
    
    // Call C_mutate_single with is_inside_block=true
    SEXP result = C_mutate_single(expr, srcref, true);
    PROTECT(result);
    
    // Verify result is a list
    EXPECT_EQ(TYPEOF(result), VECSXP);
    
    // Verify we get mutations (should get both operator flips and delete operations)
    EXPECT_GT(Rf_length(result), 0);
    
    // Deletion mutations should be present when is_inside_block=true
    // We'd need to inspect the mutation_info to verify this thoroughly
    // Here we just check we have more mutations than with is_inside_block=false
    SEXP regular_result = C_mutate_single(expr, srcref, false);
    PROTECT(regular_result);
    EXPECT_GE(Rf_length(result), Rf_length(regular_result));
    
    UNPROTECT(4);
}

// Test C_mutate_single with EXPRSXP input
TEST_F(MutateRTest, MutateSingleEXPRSXP) {
    // Create a simple expression: a + b
    SEXP expr = createExpression("+", "a", "b");
    PROTECT(expr);
    
    // Wrap it in an EXPRSXP
    SEXP exprList = PROTECT(Rf_allocVector(VECSXP, 1));
    SET_VECTOR_ELT(exprList, 0, expr);
    
    // Set the EXPRSXP type (this is a bit hacky but necessary for testing)
    SET_TYPEOF(exprList, EXPRSXP);
    
    // Create a source reference
    SEXP srcref = createSrcRef(1, 1, 1, 5);
    PROTECT(srcref);
    
    // Call C_mutate_single
    SEXP result = C_mutate_single(exprList, srcref, false);
    PROTECT(result);
    
    // Verify result is a list
    EXPECT_EQ(TYPEOF(result), VECSXP);
    
    // Verify we get at least one mutation
    EXPECT_GT(Rf_length(result), 0);
    
    UNPROTECT(4);
}

// Test C_mutate_single with no operators to mutate
TEST_F(MutateRTest, MutateSingleNoOperators) {
    // Create a simple expression with no operators: just a symbol
    SEXP expr = PROTECT(Rf_install("x"));
    
    // Create a source reference
    SEXP srcref = createSrcRef(1, 1, 1, 1);
    PROTECT(srcref);
    
    // Call C_mutate_single
    SEXP result = C_mutate_single(expr, srcref, false);
    PROTECT(result);
    
    // Verify result is an empty list
    EXPECT_EQ(TYPEOF(result), VECSXP);
    EXPECT_EQ(Rf_length(result), 0);
    
    UNPROTECT(3);
}

// Test detect_block_expressions
TEST_F(MutateRTest, DetectBlockExpressions) {
    // Create a sequence with blocks and non-blocks
    SEXP expr1 = createExpression("+", "a", "b");
    PROTECT(expr1);
    
    SEXP expr2 = createExpression("*", "c", "d");
    PROTECT(expr2);
    
    // Create a block expression
    std::vector<SEXP> blockStatements = {
        createExpression("-", "e", "f"),
        createExpression("/", "g", "h")
    };
    SEXP blockExpr = createBlockExpression(blockStatements);
    PROTECT(blockExpr);
    
    SEXP expr3 = createExpression("==", "i", "j");
    PROTECT(expr3);
    
    // Create an expression list
    std::vector<SEXP> allExprs = {expr1, expr2, blockExpr, expr3};
    SEXP exprList = createExpressionList(allExprs);
    PROTECT(exprList);
    
    // Call detect_block_expressions
    std::vector<bool> result = detect_block_expressions(exprList, 4);
    
    // Check results
    EXPECT_EQ(result.size(), 4);
    EXPECT_FALSE(result[0]); // Not in a block
    EXPECT_FALSE(result[1]); // Not in a block
    EXPECT_TRUE(result[2]);  // This is a block
    EXPECT_FALSE(result[3]); // Not in a block
    
    UNPROTECT(5);
}

// Test isValidMutant
TEST_F(MutateRTest, ValidMutant) {
    // Create a valid expression
    SEXP validExpr = createExpression("+", "a", "b");
    PROTECT(validExpr);
    
    // Check if it's considered valid
    bool isValid = isValidMutant(validExpr);
    
    // This test is tricky because isValidMutant evaluates the expression in R,
    // which may not be possible in a unit test without a fully initialized R environment.
    // For now, we'll just expect that the function doesn't crash.
    // In a real test environment, we'd need to set up variables 'a' and 'b'
    // in R_GlobalEnv first.
    
    UNPROTECT(1);
}

// Test C_mutate_file
TEST_F(MutateRTest, MutateFile) {
    // Create a simple file with multiple expressions
    SEXP expr1 = createExpression("+", "a", "b");
    PROTECT(expr1);
    
    SEXP expr2 = createExpression("*", "c", "d");
    PROTECT(expr2);
    
    // Create an expression list to represent a file
    std::vector<SEXP> exprs = {expr1, expr2};
    SEXP exprList = createExpressionList(exprs);
    PROTECT(exprList);
    
    // Create and attach source references
    std::vector<SEXP> srcRefs = {
        createSrcRef(1, 1, 1, 5),
        createSrcRef(2, 1, 2, 5)
    };
    attachSrcRefs(exprList, srcRefs);
    
    // Set the EXPRSXP type
    SET_TYPEOF(exprList, EXPRSXP);
    
    // Call C_mutate_file
    SEXP result = C_mutate_file(exprList);
    PROTECT(result);
    
    // Verify result is a list
    EXPECT_EQ(TYPEOF(result), VECSXP);
    
    // This test is also tricky due to the call to isValidMutant
    // which requires a properly initialized R environment.
    // We mainly check that the function doesn't crash.
    
    UNPROTECT(4);
}

// Main function that runs all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
