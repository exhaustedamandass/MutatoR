cmake_minimum_required(VERSION 3.10)
project(MutateR)

# Find R binary
find_program(R_EXE NAMES R)
if(NOT R_EXE)
    message(FATAL_ERROR "R not found. Please ensure R is in your PATH.")
endif()

# Query R for compilation and linking flags
execute_process(
    COMMAND "${R_EXE}" CMD config --cppflags
    OUTPUT_VARIABLE R_CPPFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND "${R_EXE}" CMD config --ldflags
    OUTPUT_VARIABLE R_LDFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Convert flags into a CMake-friendly list if needed
separate_arguments(R_CPPFLAGS)
separate_arguments(R_LDFLAGS)

# Create the shared library from your mutateR.cpp file
add_library(mutateR SHARED mutateR.cpp)

# On some platforms, R expects the shared library to have a .so extension
# and no "lib" prefix. The following property ensures no "lib" prefix is added.
set_target_properties(mutateR PROPERTIES PREFIX "")

# Add the compiler and linker flags from R
target_compile_options(mutateR PRIVATE ${R_CPPFLAGS})
target_link_libraries(mutateR PRIVATE ${R_LDFLAGS})

# If you have additional include paths or defines, you can add them like:
# target_include_directories(mutateR PRIVATE /path/to/include)
# target_compile_definitions(mutateR PRIVATE SOME_DEFINE)
