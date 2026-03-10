cmake_minimum_required(VERSION 3.13)
project(xxhash LANGUAGES CXX)

add_library(xxhash
    STATIC
        xxh_x86dispatch.c
        xxhash.c
)