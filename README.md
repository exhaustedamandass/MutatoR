# MutatoR
Automated mutation testing tool for R language

# compiling cpp source code 
g++ -g main.cpp -std=c++11 -I/usr/share/R/include -I/home/asanaliamandykov.linux/R/aarch64-unknown-linux-gnu-library/4.3/cpp11/include -L/usr/lib/R/lib -lR

R_HOME=/usr/lib/R ./a.out

# making a package

export PKG_CPPFLAGS="-std=c++11 -I/home/asanaliamandykov.linux/R/aarch64-unknown-linux-gnu-library/4.3/cpp11/include -I/usr/share/R/include"

R CMD SHLIB mutateR.cpp

R

source("mutatoR.R")

mutate_file("sample/sample.R")
