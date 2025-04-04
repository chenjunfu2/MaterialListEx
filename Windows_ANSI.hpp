#pragma once

#include <Windows.h>
#include <stdio.h>
#include <string>

template<typename U16T = char16_t>
std::basic_string<char> ConvertUtf16ToAnsi(std::basic_string<U16T> u16String)
{
	static_assert(sizeof(U16T) == sizeof(wchar_t), "U16T size must be same as wchar_t size");

	if (u16String.size() == 0)
	{
		return std::basic_string<char>{};//返回空字符串
	}

	// 获取转换后所需的缓冲区大小（包括终止符）
	int sizeNeeded = WideCharToMultiByte(//注意此函数u16接受字符数，而u8接受字节数
		CP_ACP,                // 使用当前ANSI代码页
		WC_NO_BEST_FIT_CHARS,  // 替换无法直接映射的字符
		(wchar_t*)u16String.data(),//static_assert保证底层大小相同
		u16String.size(),//主动传入大小，则转换结果不包含\0
		NULL,
		0,
		NULL,
		NULL
	);

	if (sizeNeeded == 0)
	{
		//ERROR_INSUFFICIENT_BUFFER//。 提供的缓冲区大小不够大，或者错误地设置为 NULL。
		//ERROR_INVALID_FLAGS//。 为标志提供的值无效。
		//ERROR_INVALID_PARAMETER//。 任何参数值都无效。
		//ERROR_NO_UNICODE_TRANSLATION//。 在字符串中发现无效的 Unicode。
		
		printf("\nWideCharToMultiByte failed. Error code: %lu\n", GetLastError());
		return std::basic_string<char>{};
	}

	//创建string并预分配大小
	std::basic_string<char> ansiString;
	ansiString.resize(sizeNeeded);

	// 执行实际转换
	int convertedSize = WideCharToMultiByte(
		CP_ACP,
		WC_NO_BEST_FIT_CHARS,
		(wchar_t *)u16String.data(),
		u16String.size(),
		ansiString.data(),
		ansiString.size(),
		NULL,
		NULL
	);

	if (convertedSize == 0)
	{
		printf("\nWideCharToMultiByte failed. Error code: %lu\n", GetLastError());
		return std::basic_string<char>{};
	}

	return ansiString;
}

#define ANSISTR(u16str) ConvertUtf16ToAnsi(u16str)