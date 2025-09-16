#pragma once

#include <vector>

template <typename DataType>
class NBT_Reader;

template <typename DataType>
class NBT_Writer;

template<typename Array>
class MyArray :public Array//��ʱ�����Ǳ����̳�
{
	template <typename DataType>
	friend class NBT_Reader;

	template <typename DataType>
	friend class NBT_Writer;
public:
	//�̳л��๹��
	using Array::Array;
};


#ifndef NBT_Node
#include "NBT_Node.hpp"//���include����ƭIDE������ʾ��
#endif