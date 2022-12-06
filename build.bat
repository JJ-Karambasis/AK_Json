@echo off

clang -std=c89 -O0 -g -Wextra -fdiagnostics-absolute-paths tests.c -o tests.exe