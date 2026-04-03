cmake_minimum_required(VERSION 3.13)
project(MaterialListEx)

#全局 C++/C 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

#公共编译选项
if(MSVC)
    add_compile_options(/W3)
else()
    add_compile_options(-Wall -Wextra)
endif()

#公共编译定义
add_definitions(-D_CONSOLE -DUNICODE -D_UNICODE)

#不同配置下的定义
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")

#平台检测
if(WIN32)
    set(PLATFORM_NAME "Windows")
    add_definitions(-DPLATFORM_WINDOWS=1)
elseif(APPLE)
    set(PLATFORM_NAME "Apple")
    add_definitions(-DPLATFORM_APPLE=1)
    if(IOS)
        add_definitions(-DPLATFORM_IOS=1)
    else()
        add_definitions(-DPLATFORM_MACOS=1)
    endif()
elseif(ANDROID)
    set(PLATFORM_NAME "Android")
    add_definitions(-DPLATFORM_ANDROID=1)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_NAME "Linux")
    add_definitions(-DPLATFORM_LINUX=1)
else()
    set(PLATFORM_NAME "Unknown")
    add_definitions(-DPLATFORM_UNKNOWN=1)
endif()

#架构检测
if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64|AMD64|x64)")
    set(ARCH_NAME "x64")
    add_definitions(-DARCH_X64=1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(i386|i686|x86)")
    set(ARCH_NAME "x86")
    add_definitions(-DARCH_X86=1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(aarch64|arm64)")
    set(ARCH_NAME "arm64")
    add_definitions(-DARCH_ARM64=1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    set(ARCH_NAME "arm")
    add_definitions(-DARCH_ARM=1)
else()
    set(ARCH_NAME "unknown")
    add_definitions(-DARCH_UNKNOWN=1)
endif()

#Github Workflow检测
if(DEFINED ENV{GITHUB_ACTIONS})
    message(STATUS "Building in GitHub Actions")
    add_definitions(-DGITHUB_ACTIONS)
endif()

#静态链接宏定义
macro(set_release_static_link target)
    if(MSVC)
        set_property(TARGET ${target} PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DLLDebug>"
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_options(${target} PRIVATE
            $<$<CONFIG:Release>:-static-libstdc++>
            $<$<CONFIG:Release>:-static-libgcc>
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_link_options(${target} PRIVATE
            $<$<CONFIG:Release>:-static-libstdc++>
            $<$<CONFIG:Release>:-static-libgcc>
        )
    endif()
endmacro()

#拷贝函数
function(copy_files_to_target TARGET_NAME SOURCE_FILES DEST_DIR)
    foreach(FILE_PATH ${SOURCE_FILES})
        get_filename_component(FILE_NAME ${FILE_PATH} NAME)
        set(DEST_PATH "${DEST_DIR}/${FILE_NAME}")

        add_custom_command(
            OUTPUT ${DEST_PATH}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FILE_PATH}"
                "${DEST_PATH}"
            DEPENDS "${FILE_PATH}"
            COMMENT "copy \"${FILE_PATH}\" to \"${DEST_DIR}\""
        )

        target_sources(${TARGET_NAME} PRIVATE ${DEST_PATH})
    endforeach()
endfunction()

#添加库
add_subdirectory(MaterialListEx/xxhash)
add_subdirectory(MaterialListEx/zlib)

#公共库变量（方便复用）
set(COMMON_LIBS xxhash zlib)

#全局头文件目录
include_directories(
    ${CMAKE_SOURCE_DIR}/MaterialListEx/zlib
    ${CMAKE_SOURCE_DIR}/MaterialListEx/xxhash
    ${CMAKE_SOURCE_DIR}/MaterialListEx/nbt_cpp
    ${CMAKE_SOURCE_DIR}/MaterialListEx
)

#添加项目
add_subdirectory(MaterialListEx)
