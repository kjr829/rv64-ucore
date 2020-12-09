cmake_minimum_required(VERSION 3.17)
project(lab0 C)

set(CMAKE_C_STANDARD 11)

include_directories(kern/driver)
include_directories(kern/mm)
include_directories(libs)

add_executable(lab0
        kern/driver/console.c
        kern/driver/console.h
        kern/init/init.c
        kern/libs/stdio.c
        kern/mm/memlayout.h
        kern/mm/mmu.h
        libs/defs.h
        libs/error.h
        libs/printfmt.c
        libs/readline.c
        libs/riscv.h
        libs/sbi.c
        libs/sbi.h
        libs/stdarg.h
        libs/stdio.h
        libs/string.c
        libs/string.h)
