#pragma once

#include <stdint.h>
#include <compare>

using NBT_TAG_RAW_TYPE = uint8_t;
enum class NBT_TAG : NBT_TAG_RAW_TYPE
{
	End = 0,	//������
	Byte,		//int8_t
	Short,		//int16_t
	Int,		//int32_t
	Long,		//int64_t
	Float,		//float 4byte
	Double,		//double 8byte
	Byte_Array,	//std::vector<int8_t>
	String,		//std::string->�г������ݣ���Ϊ��0��ֹ�ַ���!!
	List,		//std::list<NBT_Node>->vector
	Compound,	//std::map<std::string, NBT_Node>->�ַ���ΪNBT������
	Int_Array,	//std::vector<int32_t>
	Long_Array,	//std::vector<int64_t>
	ENUM_END,	//������ǣ����ڼ���enumԪ�ظ���
};

//���������

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