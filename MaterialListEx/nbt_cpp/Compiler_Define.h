#pragma once

//必须没有提前定义过任何此类宏
#if !defined(__COMPILER_MSVC__) &&\
	!defined(__COMPILER_GCC__) &&\
	!defined(__COMPILER_CLANG__)

	//先预定义所有可能的编译器宏
	#define __COMPILER_MSVC__ 0
	#define __COMPILER_GCC__ 0
	#define __COMPILER_CLANG__ 0
	
	//后实际判断是哪个编译器，是就替换它自己的宏为1
	#if defined(_MSC_VER)
		#undef  __COMPILER_MSVC__
		#define __COMPILER_MSVC__ 1
		#define __COMPILER_NAME__ "MSVC"
	#elif defined(__GNUC__)
		#undef  __COMPILER_GCC__
		#define __COMPILER_GCC__ 1
		#define __COMPILER_NAME__ "GCC"
	#elif defined(__clang__)
		#undef  __COMPILER_CLANG__
		#define __COMPILER_CLANG__ 1
		#define __COMPILER_NAME__ "Clang"
	#else
		#define __COMPILER_NAME__ "Unknown"
	#endif
#endif