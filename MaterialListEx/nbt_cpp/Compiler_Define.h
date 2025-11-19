#pragma once

/// @file 
/// @brief 编译器检测支持
/// 
/// 本文件定义了编译器检测宏，用于识别当前编译环境：
/// - CJF2_NBT_CPP_COMPILER_MSVC   (MSVC编译器标识)
/// - CJF2_NBT_CPP_COMPILER_GCC    (GCC编译器标识)
/// - CJF2_NBT_CPP_COMPILER_CLANG  (Clang编译器标识) 
/// - CJF2_NBT_CPP_COMPILER_NAME   (编译器名称字符串)

//先预定义所有可能的编译器宏
#define CJF2_NBT_CPP_COMPILER_MSVC 0
#define CJF2_NBT_CPP_COMPILER_GCC 0
#define CJF2_NBT_CPP_COMPILER_CLANG 0

//后实际判断是哪个编译器，是就替换它自己的宏为1
#if defined(_MSC_VER)
	#undef  CJF2_NBT_CPP_COMPILER_MSVC
	#define CJF2_NBT_CPP_COMPILER_MSVC 1
	#define CJF2_NBT_CPP_COMPILER_NAME "MSVC"
#elif defined(__GNUC__)
	#undef  CJF2_NBT_CPP_COMPILER_GCC
	#define CJF2_NBT_CPP_COMPILER_GCC 1
	#define CJF2_NBT_CPP_COMPILER_NAME "GCC"
#elif defined(__clang__)
	#undef  CJF2_NBT_CPP_COMPILER_CLANG
	#define CJF2_NBT_CPP_COMPILER_CLANG 1
	#define CJF2_NBT_CPP_COMPILER_NAME "Clang"
#else
	#define CJF2_NBT_CPP_COMPILER_NAME "Unknown"
#endif