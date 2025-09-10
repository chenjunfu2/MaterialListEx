#pragma once

#include <stdint.h>
#include <compare>

using NBT_TAG_RAW_TYPE = uint8_t;
enum class NBT_TAG : NBT_TAG_RAW_TYPE
{
	TAG_End = 0,	//������
	TAG_Byte,		//int8_t
	TAG_Short,		//int16_t
	TAG_Int,		//int32_t
	TAG_Long,		//int64_t
	TAG_Float,		//float 4byte
	TAG_Double,		//double 8byte
	TAG_Byte_Array,	//std::vector<int8_t>
	TAG_String,		//std::string->�г������ݣ���Ϊ��0��ֹ�ַ���!!
	TAG_List,		//std::list<NBT_Node>->vector
	TAG_Compound,	//std::map<std::string, NBT_Node>->�ַ���ΪNBT������
	TAG_Int_Array,	//std::vector<int32_t>
	TAG_Long_Array,	//std::vector<int64_t>
	ENUM_END,		//������ǣ����ڼ���enumԪ�ظ���
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