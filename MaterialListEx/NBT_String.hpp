#pragma once

#include <string>
#include <array>

#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"

template <typename DataType>
class NBT_Reader;

template <typename DataType>
class NBT_Writer;

template<typename String, typename StringView>
class MyString;

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

		//ע�����ﲻ���أ���ȻҪ�ж��Ƿ���mu8str
		//��Ϊһ��c����ַ�����Ȼ������mu8str��ʽ��ʼ��
		//���¼Ȱ���ĩβ0Ҳ����mu8��β

		//mutf8 string - 0xC0 0x80
		if (N >= 2 && ltrStr[N - 1] == 0x80 && ltrStr[N - 2] == 0xC0)//������Ȼ��N����Ϊǰ������Ѿ��ݼ�������������ȷ����
		{
			N -= 2;
		}

		return N;
	}

public:
	using StringView::StringView;

	template<size_t N>//c����ַ���or����
	constexpr MyStringView(const typename StringView::value_type(&ltrStr)[N]) :StringView(ltrStr, CalcStringViewSize(ltrStr, N))
	{}

	template<size_t N>//ע�⣬array����CalcStringSize��ɾ������Ҫ�Ľ�β����ΪԤ��array�������κν�β����size������
	constexpr MyStringView(const std::array<typename StringView::value_type, N> &strArray) : StringView(strArray.data(), strArray.size())
	{}

	template<typename String, typename StringView>
	constexpr explicit MyStringView(const MyString<String, StringView> &myString) : StringView(myString.data(), myString.size())//�����string��ʾ����view
	{}
};


template<typename String, typename StringView>
class MyString :public String//��ʱ�����Ǳ����̳�
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

		//ע�����ﲻ���أ���ȻҪ�ж��Ƿ���mu8str
		//��Ϊһ��c����ַ�����Ȼ������mu8str��ʽ��ʼ��
		//���¼Ȱ���ĩβ0Ҳ����mu8��β

		//mutf8 string - 0xC0 0x80
		if (N >= 2 && ltrStr[N - 1] == 0x80 && ltrStr[N - 2] == 0xC0)//������Ȼ��N����Ϊǰ������Ѿ��ݼ�������������ȷ����
		{
			N -= 2;
		}

		return N;
	}

public:
	//view����
	using View = MyStringView<StringView>;

	//�̳л��๹��
	using String::String;

	template<size_t N>//c����ַ���or����
	MyString(const typename String::value_type(&ltrStr)[N]) :String(ltrStr, CalcStringSize(ltrStr, N))
	{}

	template<size_t N>//ע�⣬array����CalcStringSize��ɾ������Ҫ�Ľ�β����ΪԤ��array�������κν�β����size������
	MyString(const std::array<typename String::value_type, N> &strArray) :String(strArray.data(), strArray.size())
	{}

	MyString(const View &view) :String(view)//�����view����string
	{}

	//ֱ�ӻ�ȡchar���͵���ͼ
	std::basic_string_view<char> GetCharTypeView(void) const
	{
		return std::basic_string_view<char>((const char *)String::data(), String::size());
	}

	auto ToCharTypeUTF8(void) const
	{
		return MUTF8_Tool<typename String::value_type, char16_t, char>::MU8ToU8(*this);//char8_t��Ϊchar
	}

	auto ToUTF8(void) const
	{
		return MUTF8_Tool<typename String::value_type, char16_t, char8_t>::MU8ToU8(*this);
	}

	auto ToUTF16(void) const
	{
		return MUTF8_Tool<typename String::value_type, char16_t, char8_t>::MU8ToU16(*this);
	}

	void FromCharTypeUTF8(std::basic_string_view<char> u8String)
	{
		*this = std::move(MUTF8_Tool<typename String::value_type, char16_t, char>::U8ToMU8(u8String));//char8_t��Ϊchar
	}

	void FromUTF8(std::basic_string_view<char8_t> u8String)
	{
		*this = std::move(MUTF8_Tool<typename String::value_type, char16_t, char8_t>::U8ToMU8(u8String));
	}

	void FromUTF16(std::basic_string_view<char16_t> u16String)
	{
		*this = std::move(MUTF8_Tool<typename String::value_type, char16_t, char8_t>::U16ToMU8(u16String));
	}
};


//��std�����ռ�����Ӵ����hash�Ա�unordered_map���Զ���ȡ
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