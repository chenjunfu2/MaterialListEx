cmake_minimum_required(VERSION 3.13)
project(zlib LANGUAGES CXX)

add_library(zlib
    STATIC
        adler32.c
        compress.c
        crc32.c
        deflate.c
        gzclose.c
        gzlib.c
        gzread.c
        gzwrite.c
        infback.c
        inffast.c
        inflate.c
        inftrees.c
        trees.c
        uncompr.c
        zutil.c
)

#添加对 POSIX 功能的支持
if(UNIX AND NOT APPLE)
    target_compile_definitions(zlib PRIVATE 
        _POSIX_C_SOURCE=200112L
        _XOPEN_SOURCE=600
    )
endif()

if(UNIX)
    target_compile_definitions(zlib PRIVATE HAVE_UNISTD_H)
endif()