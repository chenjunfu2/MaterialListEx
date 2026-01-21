#pragma once

#include <string>
#include <array>

#include "MUTF8_Tool.hpp"

/// @file
/// @brief NBT字符串类型

class NBT_Reader;
class NBT_Writer;
class NBT_Helper;

template<typename String, typename StringView>
class NBT_String;

/// @brief 继承自标准库std::basic_string_view的代理类，用来提供Modified-UTF-8 String的视图类型
/// @tparam StringView 继承的父类，也就是std::basic_string_view
/// @tparam String 与此类绑定的std::basic_string类型，用于提供互相转换功能
/// @note 用户不应自行实例化此类，请使用NBT_Type::String::View来访问此类实例化类型。
template<typename String, typename StringView>
class NBT_StringView : public StringView
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
	/// @brief 使用基类构造
	using StringView::StringView;

	/// @brief 使用基类赋值
	using StringView::operator=;

	/// @brief 使用基类下标
	using StringView::operator[];

	/// @brief 提供从父类拷贝构造此类的途径
	/// @param _Copy 需要拷贝构造的对象
	constexpr NBT_StringView(const StringView &_Copy) :StringView(_Copy)
	{}

	/// @brief 提供从父类移动构造此类的途径
	/// @param _Move 需要移动构造的对象
	constexpr NBT_StringView(StringView &&_Move) : StringView(std::move(_Move))
	{}

	/// @brief 通过c风格字符串或字符数组初始化
	/// @tparam N 数组的长度，自动计算
	/// @param ltrStr 数组的引用
	/// @note 如果字符串或字符数组以c风格字符串的\0或mutf8的0x80 0xc0结尾，则裁切多余结尾，因为string数据中不应包含字符串结束符。
	/// 这里视图的裁切仅缩小视图内存储的字符串实际大小，而非修改原始对象进行修改，
	/// 因为View仅构造为视图，不持有对象，如果引用的对象提前结束生命周期，会导致未定义行为，
	/// 请保证被View引用的对象生存周期大于View，或在生存周期结束后不再使用由其初始化的View。
	template<size_t N>//c风格字符串or数组
	constexpr NBT_StringView(const typename StringView::value_type(&ltrStr)[N]) :StringView(ltrStr, CalcStringViewSize(ltrStr, N))
	{}

	/// @brief 通过对应的NBT_String，也就是NBT_Type::String显示初始化为视图View
	/// @param myString 当前类对应的NBT_String的引用
	/// @note 显示构造的目的是防止意外的非持有性转换导致非预期的对象生命周期。
	/// 因为View仅构造为视图，不持有对象，如果引用的对象提前结束生命周期，会导致未定义行为，
	/// 请保证被View引用的对象生存周期大于View，或在生存周期结束后不再使用由其初始化的View。
	/// 同时，如果构造视图的NBT_String对象改变，则它的data结果很可能也会改变，此时再使用此对象也会导致未定义行为，请重新构造。
	constexpr explicit NBT_StringView(const NBT_String<String, StringView> &myString) : StringView(myString.data(), myString.size())
	{}

	/// @brief 删除临时对象构造方式，防止从临时对象构造导致悬空指针
	/// @param _Temp 参数未使用 临时对象的引用
	/// @note 一定程度上的防御，防止用户疏忽，但是无法完全杜绝极端情况。
	NBT_StringView(NBT_String<String, StringView> &&_Temp) = delete;

	/// @brief 直接获取char类型的视图
	/// @return 返回char类型的视图View
	/// @note 仅用于方便部分只支持char类型字符串的库使用，不要把它当成Mutf8到Utf8的字符编码转换功能。
	/// 要使用此转换，请使用NBT_String，也就是NBT_Type::String中的转换函数，或是直接通过此对象调用MUTF8_Tool中
	/// 的转换API，此类不直接提供转换API的原因在于这个类不该完成带有额外拷贝或开销的内容，它只是视图。
	std::basic_string_view<char> GetCharTypeView(void) const noexcept
	{
		return std::basic_string_view<char>((const char *)StringView::data(), StringView::size());
	}
};


/// @brief 继承自标准库std::basic_string的代理类，用于存储、处理与转换Modified-UTF-8字符串
/// @tparam String 继承的父类，也就是std::basic_string
/// @tparam StringView 与此类绑定的std::basic_string_view类型，用于提供互相转换功能
/// @note 用户不应自行实例化此类，请使用NBT_Type::String来访问此类实例化类型。
template<typename String, typename StringView>
class NBT_String :public String//暂时不考虑保护继承
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
	/// @brief 当前String对应的视图View类型
	using View = NBT_StringView<String, StringView>;

	/// @brief 使用基类构造
	using String::String;

	/// @brief 使用基类赋值
	using String::operator=;

	/// @brief 使用基类下标
	using String::operator[];

	/// @brief 通过c风格字符串或字符数组初始化
	/// @param ltrStr 数组的引用
	/// @note 如果字符串或字符数组以c风格字符串的\0或mutf8的0x80 0xc0结尾，则裁切多余结尾，因为string数据中不应包含字符串结束符，
	/// 这里的裁切仅缩小拷贝字符串长度，而非修改原始对象进行修改。
	template<size_t N>
	NBT_String(const typename String::value_type(&ltrStr)[N]) :String(ltrStr, CalcStringSize(ltrStr, N))
	{}

	/// @brief 从对应的视图类型View中构造String
	/// @param view 视图类型View的引用
	NBT_String(const View &view) :String(view)
	{}

	/// @brief 从对应的视图类型View的父类构造String
	/// @param _View 视图类型View的父类的引用
	NBT_String(const StringView &_View) :String(_View)
	{}

	/// @brief 直接从当前对象获取char类型的视图
	/// @return char类型的视图View
	/// @note 仅用于方便部分只支持char类型字符串的库使用，如果需要转换到其它字符编码，请调用本类的转换API。
	/// 在修改本对象，或离开作用域后，不应该再使用此函数原先返回的view类型。
	std::basic_string_view<char> GetCharTypeView(void) const noexcept
	{
		return std::basic_string_view<char>((const char *)String::data(), String::size());
	}

	/// @brief 转换到UTF-8字符编码，但是返回为char类型而非char8_t类型
	/// @return 自动推导，应为char类型的std::basic_string
	/// @note 返回为char类型仅用于方便部分只支持char类型的库使用，实际编码仍为UTF-8。
	/// 转换后的字符串与当前字符串对象与当前字符串对象独立，互不影响。
	auto ToCharTypeUTF8(void) const
	{
		return MUTF8_Tool<typename String::value_type, char16_t, char>::MU8ToU8(*this);//char8_t改为char
	}

	/// @brief 转换到UTF-8字符编码
	/// @return 自动推导，应为char8_t类型的std::basic_string
	/// @note 转换后的字符串与当前字符串对象与当前字符串对象独立，互不影响。
	auto ToUTF8(void) const
	{
		return MUTF8_Tool<typename String::value_type, char16_t, char8_t>::MU8ToU8(*this);
	}

	/// @brief 转换到UTF-16字符编码，但是返回为wchar_t类型而非char16_t类型
	/// @return 自动推导，应为wchar_t类型的std::basic_string
	/// @note 返回为wchar_t类型仅用于方便部分只支持wchar_t类型的库使用，实际编码仍为UTF-16。
	/// 转换后的字符串与当前字符串对象与当前字符串对象独立，互不影响。
	auto ToWchartTypeUTF16(void) const
	{
		return MUTF8_Tool<typename String::value_type, wchar_t, char8_t>::MU8ToU16(*this);//char16_t改为wchar_t
	}

	/// @brief 转换到UTF-16字符编码
	/// @return 自动推导，应为char16_t类型的std::basic_string
	/// @note 转换后的字符串与当前字符串对象与当前字符串对象独立，互不影响。
	auto ToUTF16(void) const
	{
		return MUTF8_Tool<typename String::value_type, char16_t, char8_t>::MU8ToU16(*this);
	}

	/// @brief 从char类型的UTF-8编码字符串替换当前对象内容
	/// @param u8String char类型的视图view，实际编码应为UTF-8
	/// @note 从视图中的字符串转换为Modified-UTF-8后替换当前对象。
	void FromCharTypeUTF8(std::basic_string_view<char> u8String)
	{
		String::operator=(std::move(MUTF8_Tool<typename String::value_type, char16_t, char>::U8ToMU8(u8String)));//char8_t改为char
	}

	/// @brief 从UTF-8编码字符串替换当前对象内容
	/// @param u8String char8_t类型的视图view
	/// @note 从视图中的字符串转换为Modified-UTF-8后替换当前对象。
	void FromUTF8(std::basic_string_view<char8_t> u8String)
	{
		String::operator=(std::move(MUTF8_Tool<typename String::value_type, char16_t, char8_t>::U8ToMU8(u8String)));
	}

	/// @brief 从wchar_t类型的UTF-16编码字符串替换当前对象内容
	/// @param u16String wchar_t类型的视图view，实际编码应为UTF-16
	/// @note 从视图中的字符串转换为Modified-UTF-8后替换当前对象。
	void FromWchartTypeUTF16(std::basic_string_view<wchar_t> u16String)
	{
		String::operator=(std::move(MUTF8_Tool<typename String::value_type, wchar_t, char8_t>::U16ToMU8(u16String)));//char16_t改为wchar_t
	}

	/// @brief 从UTF-16编码字符串替换当前对象内容
	/// @param u16String char16_t类型的视图view
	/// @note 从视图中的字符串转换为Modified-UTF-8后替换当前对象。
	void FromUTF16(std::basic_string_view<char16_t> u16String)
	{
		String::operator=(std::move(MUTF8_Tool<typename String::value_type, char16_t, char8_t>::U16ToMU8(u16String)));
	}
};


/// @brief 在std命名空间中添加类的默认hash特化以便unordered_map等容器自动获取
/// @note 实际hash实现使用标准库对默认char类型string的实现。
namespace std
{
	/// @brief NBT_String类，也就是NBT_Type::String在std名称空间中的hash函数重载
	/// @tparam String NBT_String类的模板，参见类说明
	/// @tparam StringView NBT_String类的模板，参见类说明
	/// @note 仅通过将底层指针转换为char类型指针并传递给原始标准库hash实现。
	template<typename String, typename StringView>
	struct hash<NBT_String<String, StringView>>
	{
		/// @brief 仿函数调用运算符
		/// @param s 需要hash的对象引用
		/// @return hash的值
		size_t operator()(const NBT_String<String, StringView> &s) const noexcept
		{
			return std::hash<std::basic_string_view<char>>{}(s.GetCharTypeView());
		}
	};

	/// @brief NBT_StringView类，也就是NBT_Type::String::View在std名称空间中的hash函数重载
	/// @tparam String NBT_StringView类的模板，参见类说明
	/// @tparam StringView NBT_StringView类的模板，参见类说明
	/// @note 仅通过将底层指针转换为char类型指针并传递给原始标准库hash实现。
	template<typename String, typename StringView>
	struct hash<NBT_StringView<String, StringView>>
	{
		/// @brief 仿函数调用运算符
		/// @param s 需要hash的对象引用
		/// @return hash的值
		size_t operator()(const NBT_StringView<String, StringView> &s) const noexcept
		{
			return std::hash<std::basic_string_view<char>>{}(s.GetCharTypeView());
		}
	};
}