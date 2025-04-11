# Makefile for MutatoR C++ unit tests

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra -g -fPIC

# R-related settings (match settings from ../src/Makefile)
R_HOME := /usr/share/R
CPP11 := /home/asanaliamandykov.linux/R/aarch64-unknown-linux-gnu-library/4.3/cpp11

# Includes and libraries
INCLUDES = -I../src -I$(CPP11)/include -I$(R_HOME)/include
R_LIBS = -L$(R_HOME)/lib -lR

# GoogleTest settings - adjust these paths as needed for your system
GTEST_DIR = /usr/include/gtest
GTEST_LIB_DIR = /usr/lib
GTEST_LIBS = -L$(GTEST_LIB_DIR) -lgtest -lgtest_main -pthread

# All operator files
OPERATOR_SOURCES = ../src/Operator.cpp \
                  ../src/PlusOperator.cpp \
                  ../src/MinusOperator.cpp \
                  ../src/DivideOperator.cpp \
                  ../src/MultiplyOperator.cpp \
                  ../src/EqualOperator.cpp \
                  ../src/NotEqualOperator.cpp \
                  ../src/LessThanOperator.cpp \
                  ../src/MoreThanOperator.cpp \
                  ../src/LessThanOrEqualOperator.cpp \
                  ../src/MoreThanOrEqualOperator.cpp \
                  ../src/AndOperator.cpp \
                  ../src/OrOperator.cpp \
                  ../src/LogicalOrOperator.cpp \
                  ../src/LogicalAndOperator.cpp \
                  ../src/DeleteOperator.cpp

# Core source files
CORE_SOURCES = ../src/ASTHandler.cpp \
               ../src/Mutator.cpp

# All source files (excluding init.c which is for R package registration)
SRC_FILES = $(CORE_SOURCES) $(OPERATOR_SOURCES)

# Test source files
TEST_SOURCES = ASTHandlerTest.cpp MutatorTest.cpp MutateRTest.cpp

# Object files
TEST_OBJECTS = $(TEST_SOURCES:.cpp=.o)
OPERATOR_OBJECTS = $(OPERATOR_SOURCES:.cpp=.o)
CORE_OBJECTS = $(CORE_SOURCES:.cpp=.o)
SRC_OBJECTS = $(SRC_FILES:.cpp=.o)

# Test executables
TEST_EXECS = ASTHandlerTest MutatorTest MutateRTest

# Main targets
all: $(TEST_EXECS)

# Rule to build test executables
ASTHandlerTest: ASTHandlerTest.o $(CORE_OBJECTS) $(OPERATOR_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GTEST_LIBS) $(R_LIBS)

MutatorTest: MutatorTest.o $(CORE_OBJECTS) $(OPERATOR_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GTEST_LIBS) $(R_LIBS)

MutateRTest: MutateRTest.o $(CORE_OBJECTS) $(OPERATOR_OBJECTS) ../src/mutateR.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GTEST_LIBS) $(R_LIBS)

# Rule to build .o files from .cpp files in the test directory
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Rule to build .o files from .cpp files in the src directory
../src/%.o: ../src/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Run all tests
test: $(TEST_EXECS)
	@echo "Running ASTHandler tests..."
	./ASTHandlerTest
	@echo "Running Mutator tests..."
	./MutatorTest
	@echo "Running MutateR tests..."
	./MutateRTest

# Clean up (only cleans test objects, not src objects to avoid conflicts with the main build)
clean:
	rm -f $(TEST_EXECS) *.o 

# Deep clean (cleans everything, including src objects)
deep-clean: clean
	rm -f ../src/*.o

.PHONY: all test clean deep-clean
