#pragma once

#include <string>
#include <array>

#include "MUTF8_Tool.hpp"

class NBT_Reader;
class NBT_Writer;
class NBT_Helper;

template<typename String, typename StringView>
class MyString;

template<typename String, typename StringView>
class MyStringView : public StringView
{
	friend class NBT_Reader;
	friend class NBT_Writer;
	friend class NBT_Helper;

private:
	static constexpr size_t CalcStringViewSize(const typename StringView::value_type *ltrStr, size_t N)
	{
		//c string - 0x00
		if (N >= 1 && ltrStr[N - 1] == 0x00)
		{
			N -= 1;
		}

		//注意这里不返回，仍然要判断是否是mu8str
		//因为一个c风格字符串仍然可以以mu8str形式初始化
		//导致既包含末尾0也包含mu8结尾

		//mutf8 string - 0xC0 0x80
		if (N >= 2 && ltrStr[N - 1] == 0x80 && ltrStr[N - 2] == 0xC0)//这里仍然用N，因为前面可能已经递减，这样就能正确访问
		{
			N -= 2;
		}

		return N;
	}

public:
	using StringView::StringView;

	template<size_t N>//c风格字符串or数组
	constexpr MyStringView(const typename StringView::value_type(&ltrStr)[N]) :StringView(ltrStr, CalcStringViewSize(ltrStr, N))
	{}

	template<size_t N>//注意，array不会CalcStringSize以删除不必要的结尾，因为预期array不包含任何结尾，以size代表长度
	constexpr MyStringView(const std::array<typename StringView::value_type, N> &strArray) : StringView(strArray.data(), strArray.size())
	{}

	constexpr explicit MyStringView(const MyString<String, StringView> &myString) : StringView(myString.data(), myString.size())//允许从string显示构造view
	{}

	//删除临时对象构造方式，防止从临时对象构造导致悬空指针
	MyStringView(MyString<String, StringView> &&) = delete;

	//直接获取char类型的视图
	std::basic_string_view<char> GetCharTypeView(void) const noexcept
	{
		return std::basic_string_view<char>((const char *)StringView::data(), StringView::size());
	}
};


template<typename String, typename StringView>
class MyString :public String//暂时不考虑保护继承
{
	friend class NBT_Reader;
	friend class NBT_Writer;
	friend class NBT_Helper;

	static_assert(sizeof(typename std::string::value_type) == sizeof(typename String::value_type), "Size error");
	
private:
	static constexpr size_t CalcStringSize(const typename String::value_type *ltrStr, size_t N)
	{
		//c string - 0x00
		if (N >= 1 && ltrStr[N - 1] == 0x00)
		{
			N -= 1;
		}

		//注意这里不返回，仍然要判断是否是mu8str
		//因为一个c风格字符串仍然可以以mu8str形式初始化
		//导致既包含末尾0也包含mu8结尾

		//mutf8 string - 0xC0 0x80
		if (N >= 2 && ltrStr[N - 1] == 0x80 && ltrStr[N - 2] == 0xC0)//这里仍然用N，因为前面可能已经递减，这样就能正确访问
		{
			N -= 2;
		}

		return N;
	}

public:
	//view类型
	using View = MyStringView<String, StringView>;

	//继承基类构造
	using String::String;

	template<size_t N>//c风格字符串or数组
	MyString(const typename String::value_type(&ltrStr)[N]) :String(ltrStr, CalcStringSize(ltrStr, N))
	{}

	template<size_t N>//注意，array不会CalcStringSize以删除不必要的结尾，因为预期array不包含任何结尾，以size代表长度
	MyString(const std::array<typename String::value_type, N> &strArray) :String(strArray.data(), strArray.size())
	{}

	MyString(const View &view) :String(view)//允许从view构造string
	{}

	//直接获取char类型的视图
	std::basic_string_view<char> GetCharTypeView(void) const noexcept
	{
		return std::basic_string_view<char>((const char *)String::data(), String::size());
	}

	auto ToCharTypeUTF8(void) const
	{
		return MUTF8_Tool<typename String::value_type, char16_t, char>::MU8ToU8(*this);//char8_t改为char
	}

	auto ToUTF8(void) const
	{
		return MUTF8_Tool<typename String::value_type, char16_t, char8_t>::MU8ToU8(*this);
	}

	auto ToWchartTypeUTF16(void) const
	{
		return MUTF8_Tool<typename String::value_type, wchar_t, char8_t>::MU8ToU16(*this);//char16_t改为wchar_t
	}

	auto ToUTF16(void) const
	{
		return MUTF8_Tool<typename String::value_type, char16_t, char8_t>::MU8ToU16(*this);
	}

	void FromCharTypeUTF8(std::basic_string_view<char> u8String)
	{
		String::operator=(std::move(MUTF8_Tool<typename String::value_type, char16_t, char>::U8ToMU8(u8String)));//char8_t改为char
	}

	void FromUTF8(std::basic_string_view<char8_t> u8String)
	{
		String::operator=(std::move(MUTF8_Tool<typename String::value_type, char16_t, char8_t>::U8ToMU8(u8String)));
	}

	void FromWchartTypeUTF16(std::basic_string_view<wchar_t> u16String)
	{
		String::operator=(std::move(MUTF8_Tool<typename String::value_type, wchar_t, char8_t>::U16ToMU8(u16String)));//char16_t改为wchar_t
	}

	void FromUTF16(std::basic_string_view<char16_t> u16String)
	{
		String::operator=(std::move(MUTF8_Tool<typename String::value_type, char16_t, char8_t>::U16ToMU8(u16String)));
	}
};


//在std命名空间中添加此类的hash以便unordered_map等自动获取
namespace std
{
	template<typename String, typename StringView>
	struct hash<MyString<String, StringView>>
	{
		size_t operator()(const MyString<String, StringView> &s) const noexcept
		{
			return std::hash<std::basic_string_view<char>>{}(s.GetCharTypeView());
		}
	};

	template<typename String, typename StringView>
	struct hash<MyStringView<String, StringView>>
	{
		size_t operator()(const MyStringView<String, StringView> &s) const noexcept
		{
			return std::hash<std::basic_string_view<char>>{}(s.GetCharTypeView());
		}
	};
}