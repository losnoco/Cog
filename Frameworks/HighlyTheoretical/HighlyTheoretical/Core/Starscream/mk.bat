@echo off
gcc -O2 -Wall star.c -s -o star.exe
star s68000.asm -fastcall -hog
nasmw -o s68000.obj -f win32 s68000.asm
