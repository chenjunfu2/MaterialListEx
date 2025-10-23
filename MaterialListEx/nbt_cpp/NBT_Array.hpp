#pragma once

#include <vector>

class NBT_Reader;
class NBT_Writer;
class NBT_Helper;

template<typename Array>
class MyArray :public Array//暂时不考虑保护继承
{
	friend class NBT_Reader;
	friend class NBT_Writer;
	friend class NBT_Helper;

public:
	//继承基类构造
	using Array::Array;
};