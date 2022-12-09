@echo off

clang -std=c89 -O0 -g -Wextra -fdiagnostics-absolute-paths -Wno-deprecated-declarations -Wno-unused-parameter tests.c -o tests.exe