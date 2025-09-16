#pragma once

#include <vector>

template <typename DataType>
class NBT_Reader;

template <typename DataType>
class NBT_Writer;

template<typename Array>
class MyArray :public Array//暂时不考虑保护继承
{
	template <typename DataType>
	friend class NBT_Reader;

	template <typename DataType>
	friend class NBT_Writer;
public:
	//继承基类构造
	using Array::Array;
};


#ifndef NBT_Node
#include "NBT_Node.hpp"//这个include用来骗IDE代码提示的
#endif