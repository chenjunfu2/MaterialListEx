#pragma once

#include <vector>

template<typename Array>
class MyArray :public Array
{
public:
	//继承基类构造
	using Array::Array;
};


#ifndef NBT_Node
#include "NBT_Node.hpp"//这个include用来骗IDE代码提示的
#endif