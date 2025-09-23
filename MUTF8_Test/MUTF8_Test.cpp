// MUTF8_Test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

/*
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

int main0()
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


consteval size_t operator""_Y(const char *Str, size_t Size)
{
	return Size;
}

template <char... Cs>
consteval size_t operator""_X(void)
{
	constexpr char arr[] = { Cs... };
	return sizeof(arr);
}


int main999(void)
{
	printf("%zu", ("test"_Y));
	123_X;

	return 0;
}
*/
//--------------------------------------------------------------------------//
#include <stdio.h>
#include <utility>

consteval size_t MyStaticFunc(const char16_t *u16String, size_t N)
{
	size_t szCounter = 0;
	for (auto it = u16String, end = u16String + N; it != end; ++it)
	{
		if (*it < 255)
		{
			++szCounter;
		}
		else
		{
			szCounter += 2;
		}
	}

	return szCounter;
}

template<size_t N>
consteval size_t MyStaticFunc(const char16_t(&u16String)[N])
{
	return MyStaticFunc(u16String, N);
}


template<size_t N>
consteval size_t Test(void)
{
	return N;
}

consteval auto Test0(void)
{
	return MyStaticFunc(u"你好");
}

consteval auto Test1(void)
{
	return Test<Test0()>();
}

template<size_t N>
consteval size_t Test2(const char16_t(&u16String)[N])
{
	return MyStaticFunc(u16String);
}

consteval size_t Test3(const char16_t *u16String, size_t N)
{
	auto tmp = MyStaticFunc(u16String, N);
	return tmp;
}

consteval size_t Test4(const char16_t *u16String, size_t N)
{
	return Test3(u16String, N);
}

//template<size_t N>
//consteval size_t Test5(const char16_t(&u16String)[N])
//{
//	return Test<Test2(u16String)>();
//}
//
//consteval size_t Test6(const char16_t *u16String, size_t N)
//{
//	return Test<Test3(u16String, N)>();
//}

//template<typename ...Args>
//consteval size_t Test7(Args... args)
//{
//	return Test<Test2(std::forward<Args>(args)...)>();
//}

//template<typename T>
//consteval size_t Test8(T&& t)
//{
//	return Test<Test2(std::forward<T>(t))>();
//}

//template <auto &Str>
//consteval size_t Test9()
//{
//	return Test<Test2(Str)>();
//}


int mainyy(void)
{
	constexpr auto t0 = Test<Test0()>();
	constexpr auto t1 = Test1();

	constexpr auto t2 = Test<Test2(u"测试，哥们")>();
	constexpr auto t3 = Test<Test3(u"测试，哥们", sizeof(u"测试，哥们") / sizeof(char16_t) - 1)>();
	constexpr auto t4 = Test<Test4(u"测试，哥们", sizeof(u"测试，哥们") / sizeof(char16_t) - 1)>();

	//constexpr auto t5 = Test5(u"不是，哥们");
	//constexpr auto t6 = Test6(u"不是，哥们", sizeof(u"不是，哥们") / sizeof(char16_t) - 1);
	//constexpr auto t7 = Test7(u"不是，哥们");
	//constexpr auto t8 = Test8(u"不是，哥们");
	//constexpr auto t9 = Test9<u"不是，哥们">();

	printf("%zu\n", t0);
	printf("%zu\n", t1);
	printf("%zu\n", t2);
	printf("%zu\n", t3);
	printf("%zu\n", t4);
	//printf("%zu\n", t5);
	//printf("%zu\n", t6);
	//printf("%zu\n", t7);
	//printf("%zu\n", t8);
	//printf("%zu\n", t9);
	return 0;
}



#include "../MaterialListEx/NBT_Node.hpp"
#include "../MaterialListEx/MUTF8_Tool.hpp"
#include "../MaterialListEx/Windows_ANSI.hpp"

#include <stdlib.h>
#include <locale.h>

//转换为静态字符串数组
//#define MU8STR_(charLiteralString) (MUTF8_Tool<char,char16_t>::U16ToMU8<MUTF8_Tool<char,char16_t>::U16ToMU8Size(u##charLiteralString)>(u##charLiteralString))

NBT_Type::String FT(const NBT_Type::String &s)
{
	return s;
}


int main(void)
{
	//NBT_Type::String t = MU8STR("test啊");
	//std::string t(std::string_view("test啊"));

	//bool b = t.ends_with(MU8STR("t啊"));

	//printf("%.*s\n%s\n", (unsigned int)t.size(), t.data(), b ? "true" : "false");

	//setlocale(LC_ALL, "zn_CN.UTF-8");
	//NBT_Type::String t(MU8STR("测试"));
	//std::string t1("测试");
	//printf("%s\n", t.c_str());
	//printf("%s\n", t1.c_str());


	//setlocale(LC_ALL, "zn_CN.UTF-8");
	//printf("%s\n", MU8STR("😂🤣").c_str());

	NBT_Type::String s(MU8STR("123123"));

	NBT_Type::String::View sv(s);


	return 0;
}