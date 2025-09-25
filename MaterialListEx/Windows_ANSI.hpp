#pragma once

#include <Windows.h>
#include <stdio.h>
#include <string>

template<typename U16T = char16_t, typename ANSIT = char>
std::basic_string<ANSIT> ConvertUtf16ToAnsi(const std::basic_string<U16T> &u16String)
{
	static_assert(sizeof(U16T) == sizeof(wchar_t), "U16T size must be same as wchar_t size");

	if (u16String.empty())
	{
		return std::basic_string<ANSIT>{};//返回空字符串
	}

	// 获取转换后所需的缓冲区大小（包括终止符）
	const int lengthNeeded = WideCharToMultiByte(//注意此函数u16接受字符数，而u8接受字节数
		CP_ACP,                // 使用当前ANSI代码页
		WC_NO_BEST_FIT_CHARS,  // 替换无法直接映射的字符
		(wchar_t*)u16String.data(),//static_assert保证底层大小相同
		u16String.size(),//主动传入大小，则转换结果不包含\0
		NULL,
		0,
		NULL,
		NULL
	);

	if (lengthNeeded == 0)
	{
		//ERROR_INSUFFICIENT_BUFFER//。 提供的缓冲区大小不够大，或者错误地设置为 NULL。
		//ERROR_INVALID_FLAGS//。 为标志提供的值无效。
		//ERROR_INVALID_PARAMETER//。 任何参数值都无效。
		//ERROR_NO_UNICODE_TRANSLATION//。 在字符串中发现无效的 Unicode。
		
		printf("\nfirst WideCharToMultiByte failed. Error code: %lu\n", GetLastError());
		return std::basic_string<ANSIT>{};
	}

	//创建string并预分配大小
	std::basic_string<ANSIT> ansiString;
	ansiString.resize(lengthNeeded);

	// 执行实际转换
	int convertedSize = WideCharToMultiByte(
		CP_ACP,
		WC_NO_BEST_FIT_CHARS,
		(wchar_t *)u16String.data(),
		u16String.size(),
		(char *)ansiString.data(),
		lengthNeeded,
		NULL,
		NULL
	);

	if (convertedSize == 0)
	{
		printf("\nsecond WideCharToMultiByte failed. Error code: %lu\n", GetLastError());
		return std::basic_string<ANSIT>{};
	}

	return ansiString;
}

template<typename U16T = char16_t, typename U8T = char8_t>
std::basic_string<U8T> ConvertUtf16ToUtf8(const std::basic_string<U16T> &u16String)
{
	static_assert(sizeof(U16T) == sizeof(wchar_t), "U16T size must be same as wchar_t size");

	if (u16String.empty())
	{
		return std::basic_string<U8T>{};//返回空字符串
	}

	// 获取转换后所需的缓冲区大小（包括终止符）
	const int lengthNeeded = WideCharToMultiByte(//注意此函数u16接受字符数，而u8接受字节数
		CP_UTF8,                // 使用CP_UTF8代码页
		WC_NO_BEST_FIT_CHARS,  // 替换无法直接映射的字符
		(wchar_t *)u16String.data(),//static_assert保证底层大小相同
		u16String.size(),//主动传入大小，则转换结果不包含\0
		NULL,
		0,
		NULL,
		NULL
	);

	if (lengthNeeded == 0)
	{
		//ERROR_INSUFFICIENT_BUFFER//。 提供的缓冲区大小不够大，或者错误地设置为 NULL。
		//ERROR_INVALID_FLAGS//。 为标志提供的值无效。
		//ERROR_INVALID_PARAMETER//。 任何参数值都无效。
		//ERROR_NO_UNICODE_TRANSLATION//。 在字符串中发现无效的 Unicode。

		printf("\nfirst WideCharToMultiByte failed. Error code: %lu\n", GetLastError());
		return std::basic_string<U8T>{};
	}

	//创建string并预分配大小
	std::basic_string<U8T> u8String;
	u8String.resize(lengthNeeded);

	// 执行实际转换
	int convertedSize = WideCharToMultiByte(
		CP_UTF8,
		WC_NO_BEST_FIT_CHARS,
		(wchar_t *)u16String.data(),
		u16String.size(),
		(char *)u8String.data(),
		lengthNeeded,
		NULL,
		NULL
	);

	if (convertedSize == 0)
	{
		printf("\nsecond WideCharToMultiByte failed. Error code: %lu\n", GetLastError());
		return std::basic_string<U8T>{};
	}

	return u8String;
}

template<typename U8T = char8_t, typename ANSIT = char>
std::basic_string<ANSIT> ConvertUtf8ToAnsi(const std::basic_string<U8T> &u8String)
{
    static_assert(sizeof(U8T) == sizeof(ANSIT), "U8T size must be same as ANSIT size");

    if (u8String.empty())
    {
        return std::basic_string<ANSIT>{};
    }

    // UTF-8 -> UTF-16
    const int lengthNeeded = MultiByteToWideChar(
        CP_UTF8,
		0,
        (char *)u8String.data(),
		u8String.size(),
        NULL,
        0
    );

    if (lengthNeeded == 0)
    {
        printf("\nfirst MultiByteToWideChar failed. Error code: %lu\n", GetLastError());
        return std::basic_string<ANSIT>{};
    }

    std::basic_string<wchar_t> utf16Str;
    utf16Str.resize(lengthNeeded);

	int convertedSize = MultiByteToWideChar(
		CP_UTF8,
		0,
		(char *)u8String.data(),
		u8String.size(),
		(wchar_t *)utf16Str.data(),
		lengthNeeded
	);

	if (convertedSize == 0)
    {
        printf("\nsecond MultiByteToWideChar failed. Error code: %lu\n", GetLastError());
        return std::basic_string<ANSIT>{};
    }

    // UTF-16 -> ANSI
	return ConvertUtf16ToAnsi<wchar_t, ANSIT>(utf16Str);
}

template<typename ANSIT = char, typename U8T = char8_t>
std::basic_string<U8T> ConvertAnsiToUtf8(const std::basic_string<ANSIT> &ansiString)
{
	static_assert(sizeof(U8T) == sizeof(ANSIT), "U8T size must be same as ANSIT size");

	if (ansiString.empty())
	{
		return std::basic_string<U8T>{};
	}

	// ANSI -> UTF-16
	const int lengthNeeded = MultiByteToWideChar(
		CP_ACP,
		0,
		(char *)ansiString.data(),
		ansiString.size(),
		NULL,
		0
	);

	if (lengthNeeded == 0)
	{
		printf("\nfirst MultiByteToWideChar failed. Error code: %lu\n", GetLastError());
		return std::basic_string<U8T>{};
	}

	std::basic_string<wchar_t> utf16Str;
	utf16Str.resize(lengthNeeded);

	int convertedSize = MultiByteToWideChar(
		CP_ACP,
		0,
		(char *)ansiString.data(),
		ansiString.size(),
		(wchar_t *)utf16Str.data(),
		lengthNeeded
	);

	if (convertedSize == 0)
	{
		printf("\nsecond MultiByteToWideChar failed. Error code: %lu\n", GetLastError());
		return std::basic_string<U8T>{};
	}

	// UTF-16 -> UTF-8
	return ConvertUtf16ToUtf8<wchar_t, U8T>(utf16Str);
}