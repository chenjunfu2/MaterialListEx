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
	MyString(const std::array<typename String::value_type, N> &strArray) : String(strArray.data(), strArray.size())
	{}
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