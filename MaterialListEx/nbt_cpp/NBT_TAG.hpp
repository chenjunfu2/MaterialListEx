#pragma once

#include <stdint.h>
#include <stddef.h>//size_t
#include <compare>

/// @file

/// @brief NBT_TAG的原始类型，用于二进制读写或判断等
/// @note 可以和NBT_TAG无损互转
using NBT_TAG_RAW_TYPE = uint8_t;

/// @brief 枚举NBT类型对应的类型标签值
/// @note 枚举值ENUM_END不应该被当作合法的类型枚举值使用，这是一个结束标记，不是合法的类型标签
enum class NBT_TAG : NBT_TAG_RAW_TYPE
{
	End = 0,	///< 对应NBT_Type::End
	Byte,		///< 对应NBT_Type::Byte
	Short,		///< 对应NBT_Type::Short
	Int,		///< 对应NBT_Type::Int
	Long,		///< 对应NBT_Type::Long
	Float,		///< 对应NBT_Type::Float
	Double,		///< 对应NBT_Type::Double
	ByteArray,	///< 对应NBT_Type::ByteArray
	String,		///< 对应NBT_Type::String
	List,		///< 对应NBT_Type::List
	Compound,	///< 对应NBT_Type::Compound
	IntArray,	///< 对应NBT_Type::IntArray
	LongArray,	///< 对应NBT_Type::LongArray
	ENUM_END,	///< 枚举结束标记，用于计算enum元素个数，范围判断等
};

//运算符重载

/// @name 用于方便enum class比较的运算符重载
/// @{

/// @brief 比较任意类型与NBT_TAG是否相等
/// @tparam T 可与NBT_TAG_RAW_TYPE比较的类型
/// @param l 左操作数（任意类型）
/// @param r 右操作数（NBT_TAG枚举值）
/// @return 如果l等于r的底层整数值则返回true
template<typename T>
constexpr bool operator==(T l, NBT_TAG r)
{
	return l == (NBT_TAG_RAW_TYPE)r;
}

/// @brief 比较NBT_TAG与任意类型是否相等
/// @tparam T 可与NBT_TAG_RAW_TYPE比较的类型
/// @param l 左操作数（NBT_TAG枚举值）
/// @param r 右操作数（任意类型）
/// @return 如果l的底层整数值等于r则返回true
template<typename T>
constexpr bool operator==(NBT_TAG l, T r)
{
	return (NBT_TAG_RAW_TYPE)l == r;
}

/// @brief 比较两个NBT_TAG枚举值是否相等
/// @param l 左操作数
/// @param r 右操作数
/// @return 如果两个枚举值的底层整数值相等则返回true
constexpr bool operator==(NBT_TAG l, NBT_TAG r)
{
	return (NBT_TAG_RAW_TYPE)l == (NBT_TAG_RAW_TYPE)r;
}

/// @brief 比较任意类型与NBT_TAG是否不相等
/// @tparam T 可与NBT_TAG_RAW_TYPE比较的类型
/// @param l 左操作数（任意类型）
/// @param r 右操作数（NBT_TAG枚举值）
/// @return 如果l不等于r的底层整数值则返回true
template<typename T>
constexpr bool operator!=(T l, NBT_TAG r)
{
	return l != (NBT_TAG_RAW_TYPE)r;
}

/// @brief 比较NBT_TAG与任意类型是否不相等
/// @tparam T 可与NBT_TAG_RAW_TYPE比较的类型
/// @param l 左操作数（NBT_TAG枚举值）
/// @param r 右操作数（任意类型）
/// @return 如果l的底层整数值不等于r则返回true
template<typename T>
constexpr bool operator!=(NBT_TAG l, T r)
{
	return (NBT_TAG_RAW_TYPE)l == r;
}

/// @brief 比较两个NBT_TAG枚举值是否不相等
/// @param l 左操作数
/// @param r 右操作数
/// @return 如果两个枚举值的底层整数值不相等则返回true
constexpr bool operator!=(NBT_TAG l, NBT_TAG r)
{
	return (NBT_TAG_RAW_TYPE)l != (NBT_TAG_RAW_TYPE)r;
}

/// @brief 三路比较任意类型与NBT_TAG的大小关系
/// @tparam T 可与NBT_TAG_RAW_TYPE比较的类型
/// @param l 左操作数（任意类型）
/// @param r 右操作数（NBT_TAG枚举值）
/// @return 表示底层整数值比较结果的std::strong_ordering枚举值
template<typename T>
constexpr std::strong_ordering operator<=>(T l, NBT_TAG r)
{
	return l <=> (NBT_TAG_RAW_TYPE)r;
}

/// @brief 三路比较NBT_TAG与任意类型的大小关系
/// @tparam T 可与NBT_TAG_RAW_TYPE比较的类型
/// @param l 左操作数（NBT_TAG枚举值）
/// @param r 右操作数（任意类型）
/// @return 表示底层整数值比较结果的std::strong_ordering枚举值
template<typename T>
constexpr std::strong_ordering operator<=>(NBT_TAG l, T r)
{
	return (NBT_TAG_RAW_TYPE)l <=> r;
}

/// @brief 三路比较两个NBT_TAG枚举值的大小关系
/// @param l 左操作数
/// @param r 右操作数
/// @return 表示底层整数值比较结果的std::strong_ordering枚举值
constexpr std::strong_ordering operator<=>(NBT_TAG l, NBT_TAG r)
{
	return (NBT_TAG_RAW_TYPE)l <=> (NBT_TAG_RAW_TYPE)r;
}

/// @}