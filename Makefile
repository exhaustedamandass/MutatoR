# Makefile

# Compiler and Flags
CXX = g++
CXXFLAGS = -O2 -std=c++14 -fPIC -I$(CPP11)/include -I$(R_HOME)/include -I./include

# Detect R_HOME automatically
R_HOME := /usr/share/R

CPP11 := /home/asanaliamandykov.linux/R/aarch64-unknown-linux-gnu-library/4.3/cpp11

# R dynamic library flags
LIBS = -L$(R_HOME)/lib -lR

# Source Files
SOURCES = src/ASTHandler.cpp \
		  src/mutateR.cpp \
          src/Mutator.cpp \
		  src/PlusOperator.cpp \
		  src/MinusOperator.cpp \
		  src/DivideOperator.cpp \
		  src/MultiplyOperator.cpp \

# Object Files
OBJECTS = $(SOURCES:.cpp=.o)

# Target Shared Library
TARGET = mutateR.so

# Default Rule
all: $(TARGET)

# Compile the Shared Library
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(LIBS)

# Compile .cpp to .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean Up
clean:
	rm -f src/*.o $(TARGET)

.PHONY: all clean
