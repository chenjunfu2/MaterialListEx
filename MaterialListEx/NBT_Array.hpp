#pragma once

#include <vector>

template<typename Array>
class MyArray :public Array
{
public:
	//�̳л��๹��
	using Array::Array;
};


#ifndef NBT_Node
#include "NBT_Node.hpp"//���include����ƭIDE������ʾ��
#endif