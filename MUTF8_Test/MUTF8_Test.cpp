// MUTF8_Test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>
#include "../MaterialListEx/MUTF8_Tool.hpp"
#include "../MaterialListEx/Windows_ANSI.hpp"

std::basic_string<wchar_t> generate_all_valid_utf16le()
{
	std::basic_string<wchar_t> result;

	// 添加非代理项平面字符（基本多文种平面）
	for (uint32_t c = 0x0000; c <= 0xD7FF; ++c)
	{
		result.push_back(static_cast<wchar_t>(c));
	}

	// 添加基本多文种平面剩余字符
	for (uint32_t c = 0xE000; c <= 0xFFFF; ++c)
	{
		result.push_back(static_cast<wchar_t>(c));
	}

	// 添加代理项对（辅助平面字符）
	for (uint32_t high = 0xD800; high <= 0xDBFF; ++high)
	{
		for (uint32_t low = 0xDC00; low <= 0xDFFF; ++low)
		{
			result.push_back(static_cast<wchar_t>(high));
			result.push_back(static_cast<wchar_t>(low));
		}
	}

	return result;
}

int main()
{
	char arrTest[] =
	{
		0xE6,0xB5,0x8B,0xE8,0xAF,0x95,
	};

	std::string sTest{ arrTest,sizeof(arrTest) / sizeof(arrTest[0]) };
	auto ansiString = ConvertUtf16ToAnsi(MUTF8_Tool<char, wchar_t>::MU8ToU16(sTest));
	printf("%s", ansiString.c_str());

	return 0;


	printf("generate_all_valid_utf16le\n");
	auto test = generate_all_valid_utf16le();
	printf("generate_all_valid_utf16le ok\n");

	printf("U16ToMU8\n");
	auto test1 = MUTF8_Tool<char, wchar_t>::U16ToMU8(test);
	printf("U16ToMU8 ok\n");

	printf("MU8ToU16\n");
	auto test2 = MUTF8_Tool<char, wchar_t>::MU8ToU16(test1);
	printf("MU8ToU16 ok\n");

	if (test != test2)
	{
		printf("test fail\n");
	}
	else
	{
		printf("test ok\n");
	}


	return 0;
}