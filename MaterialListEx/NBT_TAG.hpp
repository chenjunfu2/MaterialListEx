#pragma once

#include <stdint.h>
#include <stddef.h>//size_t
#include <compare>

using NBT_TAG_RAW_TYPE = uint8_t;
enum class NBT_TAG : NBT_TAG_RAW_TYPE
{
	End = 0,	//结束项
	Byte,		//int8_t
	Short,		//int16_t
	Int,		//int32_t
	Long,		//int64_t
	Float,		//float 4byte
	Double,		//double 8byte
	ByteArray,	//std::vector<int8_t>
	String,		//std::string->有长度数据，且为非0终止字符串!!
	List,		//std::list<NBT_Node>->vector
	Compound,	//std::map<std::string, NBT_Node>->字符串为NBT项名称
	IntArray,	//std::vector<int32_t>
	LongArray,	//std::vector<int64_t>
	ENUM_END,	//结束标记，用于计算enum元素个数
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

constexpr bool operator==(NBT_TAG l, NBT_TAG r)
{
	return (NBT_TAG_RAW_TYPE)l == (NBT_TAG_RAW_TYPE)r;
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

constexpr bool operator!=(NBT_TAG l, NBT_TAG r)
{
	return (NBT_TAG_RAW_TYPE)l != (NBT_TAG_RAW_TYPE)r;
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

constexpr std::strong_ordering operator<=>(NBT_TAG l, NBT_TAG r)
{
	return (NBT_TAG_RAW_TYPE)l <=> (NBT_TAG_RAW_TYPE)r;
}