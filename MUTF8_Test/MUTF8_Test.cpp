// MUTF8_Test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//


#include <stdio.h>
#include "../MaterialListEx/nbt/MUTF8_Tool.hpp"
#include "../MaterialListEx/Windows_ANSI.hpp"
#include <clocale>

std::basic_string<char16_t> generate_all_valid_utf16le()
{
	std::basic_string<char16_t> result;

	// 添加非代理项平面字符（基本多文种平面）
	for (uint32_t c = 0x0000; c <= 0xD7FF; ++c)
	{
		result.push_back(static_cast<char16_t>(c));
	}

	// 添加基本多文种平面剩余字符
	for (uint32_t c = 0xE000; c <= 0xFFFF; ++c)
	{
		result.push_back(static_cast<char16_t>(c));
	}

	// 添加代理项对（辅助平面字符）
	for (uint32_t high = 0xD800; high <= 0xDBFF; ++high)
	{
		for (uint32_t low = 0xDC00; low <= 0xDFFF; ++low)
		{
			result.push_back(static_cast<char16_t>(high));
			result.push_back(static_cast<char16_t>(low));
		}
	}

	return result;
}


void PrintHex(const char *ps, const std::basic_string_view<char> &s, const char *pe = "\n")
{
	printf("%s", ps);
	for (const auto &c : s)
	{
		printf("0x%02X ", (uint8_t)c);
	}
	printf("%s", pe);
}

void PrintHex(const char *ps, const std::basic_string_view<char16_t> &s, const char *pe = "\n")
{
	printf("%s", ps);
	for (const auto &c : s)
	{
		printf("0x%04X ", (uint16_t)c);
	}
	printf("%s", pe);
}

// 测试 UTF-16 → MUTF-8
template<size_t iN, size_t eN>
void TestU16ToMU8Impl(size_t szLine, const char16_t(&input)[iN], const char(&expected)[eN])
{
	auto result = MUTF8_Tool<char, char16_t, char8_t>::U16ToMU8(input, iN - 1);
	if (result != std::basic_string_view<char>(expected, eN - 1))
	{
		printf("[%zu] U16ToMU8 FAILED\n", szLine);
		PrintHex("Input   : ", input);
		PrintHex("Expected: ", expected);
		PrintHex("Got     : ", result);
		printf("\n");
	}
}

// 测试 MUTF-8 → UTF-16
template<size_t iN, size_t eN>
void TestMU8ToU16Impl(size_t szLine, const char(&input)[iN], const char16_t(&expected)[eN])
{
	auto result = MUTF8_Tool<char, char16_t, char8_t>::MU8ToU16(input, iN - 1);
	if (result != std::basic_string_view<char16_t>(expected, eN - 1))
	{
		printf("[%zu] MU8ToU16 FAILED\n", szLine);
		PrintHex("Input   : ", input);
		PrintHex("Expected: ", expected);
		PrintHex("Got     : ", result);
		printf("\n");
	}
}

#define rp_line __LINE__

#define TestU16ToMU8(i,e) TestU16ToMU8Impl(rp_line,i,e)
#define TestMU8ToU16(i,e) TestMU8ToU16Impl(rp_line,i,e)

int main(void)
{
	//const auto s = u"\xD000\xD001\xD002\xD003\xD004\xD005";
	//auto tmp = MUTF8_Tool<char, char16_t, char>::U16ToMU8(s);
	//auto tmp1 = MUTF8_Tool<char, char16_t, char>::MU8ToU16(tmp);
	//
	//printf("%d\n", s == tmp1);
	

	//const auto tmp = "\xEF\xBF\xBF""\xED\xA0\x80\xED\xB0\x80""\xED\xA0\x80\xED\xB0\x81""\xED\xA0\x80\xED\xB0\x82";
	//
	//auto tmp2 = MUTF8_Tool<char, char16_t, char>::MU8ToU8(tmp);
	//auto tmp3 = MUTF8_Tool<char, char16_t, char>::U8ToMU8(tmp2);
	//
	//printf("%d\n", tmp == tmp3);
	//
	//return 0;

	setlocale(LC_ALL, "zh_CN.UTF-8");


	auto HexPrint = [](auto data, size_t beg = 0, size_t end = (size_t)-1) -> void
	{
		uint8_t i = 0;
		using ut = std::make_unsigned_t<std::remove_reference_t<decltype(data[0])>>;

		if (beg >= end || beg >= data.size())
		{
			return;
		}

		if (end >= data.size())
		{
			end = data.size();
		}

		for (size_t i = beg; i < end; ++i)
		{
			printf("[%zu~%zu] 0x%02X ", i, i + 7, (ut)data[i]);
			if (++i == 8)
			{
				printf("\n");
				i = 0;
			}
		}
		printf("\n");
	};

	auto BoolPrint = [](bool b, const char *pBeg = "", const char *pEnd = " ") -> void
	{
		printf("%s%s%s", pBeg, b ? "true" : "false", pEnd);
	};

	auto TestPrint = [](auto l, auto r) ->void
	{
		if (l != r)
		{
			printf("test fail\n");
			auto min = min(l.size(), r.size());
			using utl = std::make_unsigned_t<std::remove_reference_t<decltype(l[0])>>;
			using utr = std::make_unsigned_t<std::remove_reference_t<decltype(r[0])>>;
			for (size_t i = 0, c = 0; i < min && c < 16; ++i)
			{
				if (l[i] != r[i])
				{
					printf("[%zu]: [%d:0x%02X] != [%d:0x%02X]\n", i, (utl)l[i], (utl)l[i], (utr)r[i], (utr)r[i]);
					++c;
				}
			}
		}
		else
		{
			printf("test ok\n");
		}
	};


	std::basic_string_view testu8 = u8"test😀😁😂🤣😃😄😅😆😗😘🥰😍😎😋😊😉😙😚☺🙂🤗🤩🤔🤨223$$在明确是常量表达式的上下文中，不需要检查是否在常量求值中，因为答案总是\"是\"";
	std::basic_string_view testu16 = u"test😀😁😂🤣😃😄😅😆😗😘🥰😍😎😋😊😉😙😚☺🙂🤗🤩🤔🤨223$$在明确是常量表达式的上下文中，不需要检查是否在常量求值中，因为答案总是\"是\"";

	constexpr auto ret = U8TOMU8STR(u8"test😀😁😂🤣😃😄😅😆😗😘🥰😍😎😋😊😉😙😚☺🙂🤗🤩🤔🤨223$$在明确是常量表达式的上下文中，不需要检查是否在常量求值中，因为答案总是\"是\"");
	auto ret1 = U8CV2MU8(testu8);
	constexpr auto ret2 = U16TOMU8STR(u"test😀😁😂🤣😃😄😅😆😗😘🥰😍😎😋😊😉😙😚☺🙂🤗🤩🤔🤨223$$在明确是常量表达式的上下文中，不需要检查是否在常量求值中，因为答案总是\"是\"");
	auto ret3 = U16CV2MU8(testu16);

	std::basic_string_view<uint8_t> mu8_0(ret.data(), ret.size());
	std::basic_string_view<uint8_t> mu8_1(ret2.data(), ret2.size());

	TestPrint(mu8_0, mu8_1);

	BoolPrint(ret == ret2, "ret == ret2: ", "\n");
	BoolPrint(ret1 == ret3, "ret1 == ret3: ", "\n\n");

	BoolPrint(mu8_0 == ret1, "mu8_0 == ret1: ", "\n");
	BoolPrint(mu8_1 == ret3, "mu8_1 == ret3: ", "\n\n");
	

	auto u16_0 = MU8CV2U16(mu8_0);
	auto u16_1 = MU8CV2U16(mu8_1);

	auto mu8_2 = U16CV2MU8(u16_0);
	auto mu8_3 = U16CV2MU8(u16_1);

	auto u8_0 = MU8CV2U8(mu8_0);
	auto u8_1 = MU8CV2U8(mu8_1);
	auto u8_2 = MU8CV2U8(mu8_2);
	auto u8_3 = MU8CV2U8(mu8_3);

	BoolPrint(testu16 == u16_0, "testu16 == u16_0: ", "\n");
	BoolPrint(testu16 == u16_1, "testu16 == u16_1: ", "\n\n");
	BoolPrint(testu8 == u8_0, "testu8 == u8_0: ", "\n");
	BoolPrint(testu8 == u8_1, "testu8 == u8_1: ", "\n");
	BoolPrint(testu8 == u8_2, "testu8 == u8_2: ", "\n");
	BoolPrint(testu8 == u8_3, "testu8 == u8_3: ", "\n");

	printf("\n\n");

	printf("u16_0:");
	TestPrint(testu16, u16_0);
	printf("u16_1:");
	TestPrint(testu16, u16_1);
	printf("u8_0:");
	TestPrint(testu8, u8_0);
	printf("u8_1:");
	TestPrint(testu8, u8_1);
	printf("u8_2:");
	TestPrint(testu8, u8_2);
	printf("u8_3:");
	TestPrint(testu8, u8_3);

	//return -1145;
	//U8TOMU8STR(u8"\xEF\xBF\xBF""\xED\xA0\x80\xED\xB0\x80""\xED\xA0\x80\xED\xB0\x81""\xED\xA0\x80\xED\xB0\x82");
	//U16TOMU8STR(u"\xEF\xBF\xBF""\xED\xA0\x80\xED\xB0\x80""\xED\xA0\x80\xED\xB0\x81""\xED\xA0\x80\xED\xB0\x82");

	printf("\n\ngenerate_all_valid_utf16le\n");
	const auto test = generate_all_valid_utf16le();
	printf("generate_all_valid_utf16le ok\n");

	printf("U16ToMU8\n");
	const auto test1 = MUTF8_Tool<char, char16_t, char>::U16ToMU8(test);
	printf("U16ToMU8 ok\n");

	printf("MU8ToU16\n");
	const auto test2 = MUTF8_Tool<char, char16_t, char>::MU8ToU16(test1);
	printf("MU8ToU16 ok\n");

	TestPrint(test, test2);

	printf("MU8ToU8\n");
	const auto test3 = MUTF8_Tool<char, char16_t, char>::MU8ToU8(test1);
	printf("MU8ToU8 ok\n");

	printf("U8ToMU8\n");
	const auto test4 = MUTF8_Tool<char, char16_t, char>::U8ToMU8(test3);
	printf("U8ToMU8 ok\n");

	TestPrint(test1, test4);

	printf("\n");


	// UTF-16 → MUTF-8 测试
	TestU16ToMU8(u"\u0041", "\x41");                        // 'A'
	TestU16ToMU8(u"\u0000", "\xC0\x80");                    // NUL 特殊编码
	TestU16ToMU8(u"\xD800", "\xEF\xBF\xBD");                // 孤立高代理
	TestU16ToMU8(u"\xDC00", "\xEF\xBF\xBD");                // 孤立低代理
	TestU16ToMU8(u"\xD800\u0041", "\xEF\xBF\xBD\x41");      // 高代理+普通字符
	TestU16ToMU8(u"\u0041\xDC00", "\x41\xEF\xBF\xBD");      // 普通字符+低代理
	TestU16ToMU8(u"\U00010000", "\xED\xA0\x80\xED\xB0\x80");// U+10000
	TestU16ToMU8(u"\U0010FFFF", "\xED\xAF\xBF\xED\xBF\xBF");// U+10FFFF
	TestU16ToMU8(u"\xD800\xD800", "\xEF\xBF\xBD\xEF\xBF\xBD");// 两个高代理

	// 混合测试（UTF-16 → MUTF-8）
	TestU16ToMU8(u"A\xD800" u"B\xDC00" u"C",
		"\x41\xEF\xBF\xBD\x42\xEF\xBF\xBD\x43");   // A  [高代理→�]  B [低代理→�] C

	TestU16ToMU8(u"\xD800\U00010000\xDC00",
		"\xEF\xBF\xBD\xED\xA0\x80\xED\xB0\x80\xEF\xBF\xBD"); // 孤立高代理 → U+FFFD, 合法代理对 → 6字节, 孤立低代理 → U+FFFD

	// MUTF-8 → UTF-16 测试
	TestMU8ToU16("\x41", u"\u0041");                        // 'A'
	TestMU8ToU16("\xC0\x80", u"\u0000");                    // 特殊 NUL
	TestMU8ToU16("\xED\xA0\x80\xED\xB0\x80", u"\U00010000");// U+10000
	TestMU8ToU16("\xED\xAF\xBF\xED\xBF\xBF", u"\U0010FFFF");// U+10FFFF
	TestMU8ToU16("\xE0\x80\x80", u"\u0000");                // 过长编码
	TestMU8ToU16("\xF4\x90\x80\x80", u"\uFFFD\uFFFD\uFFFD\uFFFD");// 超范围
	TestMU8ToU16("\xE2\x28\xA1", u"\uFFFD\u0028\uFFFD");    // 非法字节
	TestMU8ToU16("\x80", u"\uFFFD");                        // 孤立续字节
	TestMU8ToU16("\xED\xA0\x41", u"\uFFFD\uFFFD\u0041");    // 断掉的代理序列

	// 混合测试（MUTF-8 → UTF-16）
	TestMU8ToU16("\x41\xED\xA0\x80\xED\xB0\x80\x42",
		u"\u0041\U00010000\u0042"); // "A U+10000 B"

	TestMU8ToU16("\x41\xED\xA0\x41\x42",
		u"\u0041\uFFFD\uFFFD\u0041\u0042"); // "A [坏代理→�] A B"

	TestMU8ToU16("\xED\xA0\x80\xED\xA0\x80",
		u"\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD\uFFFD"); // 两个高代理编码形式 → 两个 U+FFFD

	printf("TestU16ToMU8 | TestMU8ToU16\n");

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


int maintttt(void)
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