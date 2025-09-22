#pragma once

#include <string>
#include <array>

#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"

template <typename DataType>
class NBT_Reader;

template <typename DataType>
class NBT_Writer;

template<typename StringView>
class MyStringView : public StringView
{
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
};



template<typename String, typename StringView>
class MyString :public String//暂时不考虑保护继承
{
	template <typename DataType>
	friend class NBT_Reader;

	template <typename DataType>
	friend class NBT_Writer;
	
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
	using View = MyStringView<StringView>;

	//继承基类构造
	using String::String;

	template<size_t N>//c风格字符串or数组
	MyString(const typename String::value_type(&ltrStr)[N]) :String(ltrStr, CalcStringSize(ltrStr, N))
	{}

	template<size_t N>//注意，array不会CalcStringSize以删除不必要的结尾，因为预期array不包含任何结尾，以size代表长度
	MyString(const std::array<typename String::value_type, N> &strArray) : String(strArray.data(), strArray.size())
	{}
};


//在std命名空间中添加此类的hash以便unordered_map等自动获取
namespace std
{
	template<typename T, typename StringView>
	struct hash<MyString<T, StringView>>
	{
		size_t operator()(const MyString<T, StringView> &s) const noexcept
		{
			return std::hash<T>{}(s);
		}
	};
}