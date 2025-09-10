#pragma once

#include <stdint.h>
#include <compare>

using NBT_TAG_RAW_TYPE = uint8_t;
enum class NBT_TAG : NBT_TAG_RAW_TYPE
{
	TAG_End = 0,	//结束项
	TAG_Byte,		//int8_t
	TAG_Short,		//int16_t
	TAG_Int,		//int32_t
	TAG_Long,		//int64_t
	TAG_Float,		//float 4byte
	TAG_Double,		//double 8byte
	TAG_Byte_Array,	//std::vector<int8_t>
	TAG_String,		//std::string->有长度数据，且为非0终止字符串!!
	TAG_List,		//std::list<NBT_Node>->vector
	TAG_Compound,	//std::map<std::string, NBT_Node>->字符串为NBT项名称
	TAG_Int_Array,	//std::vector<int32_t>
	TAG_Long_Array,	//std::vector<int64_t>
	ENUM_END,		//结束标记，用于计算enum元素个数
};

//运算符重载

template<typename T>
constexpr bool operator==(T l, NBT_TAG r)
{
	return l == (NBT_TAG_RAW_TYPE)r;
}

template<typename T>
constexpr bool operator==(NBT_TAG l, T r)
{
	return (NBT_TAG_RAW_TYPE)l == r;
}

template<typename T>
constexpr bool operator!=(T l, NBT_TAG r)
{
	return l != (NBT_TAG_RAW_TYPE)r;
}

template<typename T>
constexpr bool operator!=(NBT_TAG l, T r)
{
	return (NBT_TAG_RAW_TYPE)l == r;
}

template<typename T>
constexpr std::strong_ordering operator<=>(T l, NBT_TAG r)
{
	return l <=> (NBT_TAG_RAW_TYPE)r;
}

template<typename T>
constexpr std::strong_ordering operator<=>(NBT_TAG l, T r)
{
	return (NBT_TAG_RAW_TYPE)l <=> r;
}