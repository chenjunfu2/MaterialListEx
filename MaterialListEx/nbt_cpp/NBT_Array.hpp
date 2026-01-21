#pragma once

#include <vector>

/// @file
/// @brief NBT数组基础类型

class NBT_Reader;
class NBT_Writer;
class NBT_Helper;

/// @brief 继承自标准库std::vector的代理类。
/// 无特殊成员，构造与使用方式与标准库std::vector一致。
/// @tparam Array 需要继承的父类类型
/// @note 用户不应自行实例化此类，请使用NBT_Type::xxxArray来访问此类实例化类型（xxx为具体Array存储的类型）
template<typename Array>
class NBT_Array :public Array//暂时不考虑保护继承
{
	friend class NBT_Reader;
	friend class NBT_Writer;
	friend class NBT_Helper;

public:
	/// @brief 使用基类构造
	using Array::Array;

	/// @brief 使用基类赋值
	using Array::operator=;

	/// @brief 使用基类下标
	using Array::operator[];
};