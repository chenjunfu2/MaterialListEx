#pragma once

#include <string>

#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"

template <typename DataType>
class NBT_Reader;

template <typename DataType>
class NBT_Writer;

template<typename String>
class MyString :public String//暂时不考虑保护继承
{
	template <typename DataType>
	friend class NBT_Reader;

	template <typename DataType>
	friend class NBT_Writer;
public:
	//继承基类构造
	using String::String;
};

//在std命名空间中添加此类的hash以便map自动获取
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


#ifndef NBT_Node
#include "NBT_Node.hpp"//这个include用来骗IDE代码提示的
#endif