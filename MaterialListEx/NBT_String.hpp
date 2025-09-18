#pragma once

#include <string>

#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"

template <typename DataType>
class NBT_Reader;

template <typename DataType>
class NBT_Writer;

template<typename String>
class MyString :public String//��ʱ�����Ǳ����̳�
{
	template <typename DataType>
	friend class NBT_Reader;

	template <typename DataType>
	friend class NBT_Writer;
public:
	//�̳л��๹��
	using String::String;
};

//��std�����ռ�����Ӵ����hash�Ա�unordered_map���Զ���ȡ
namespace std
{
	template<typename T>
	struct hash<MyString<T>>
	{
		size_t operator()(const MyString<T> &s) const noexcept
		{
			return std::hash<T>{}(s);
		}
	};
}