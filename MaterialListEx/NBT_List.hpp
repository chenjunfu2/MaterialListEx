#pragma once

#include <vector>
#include <compare>
#include <type_traits>

#include "NBT_TAG.hpp"

template<typename List>
class MyList :public List
{
public:
	//继承基类构造
	using List::List;

	//简化list查询
	inline typename List::value_type &Get(const typename List::size_type &szPos)
	{
		return List::at(szPos);
	}

	inline const typename List::value_type &Get(const typename List::size_type &szPos) const
	{
		return List::at(szPos);
	}

#define TYPE_GET_FUNC(type)\
inline const typename List::value_type::NBT_##type &Get##type(const typename List::size_type &szPos) const\
{\
	return List::at(szPos).Get##type();\
}\
\
inline typename List::value_type::NBT_##type &Get##type(const typename List::size_type &szPos)\
{\
	return List::at(szPos).Get##type();\
}


	TYPE_GET_FUNC(End);
	TYPE_GET_FUNC(Byte);
	TYPE_GET_FUNC(Short);
	TYPE_GET_FUNC(Int);
	TYPE_GET_FUNC(Long);
	TYPE_GET_FUNC(Float);
	TYPE_GET_FUNC(Double);
	TYPE_GET_FUNC(ByteArray);
	TYPE_GET_FUNC(IntArray);
	TYPE_GET_FUNC(LongArray);
	TYPE_GET_FUNC(String);
	TYPE_GET_FUNC(List);
	TYPE_GET_FUNC(Compound);


#undef TYPE_GET_FUNC

};

#ifndef NBT_Node
#include "NBT_Node.hpp"//这个include用来骗IDE代码提示的
#endif