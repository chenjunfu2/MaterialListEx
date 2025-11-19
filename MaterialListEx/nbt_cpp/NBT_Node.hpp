#pragma once

#include <compare>

#include "NBT_Type.hpp"

#include "NBT_Array.hpp"
#include "NBT_String.hpp"
#include "NBT_List.hpp"
#include "NBT_Compound.hpp"

/// @file

//在这里，所有依赖的定义都已完备，给出方便调用获取字符串字面量的的宏定义
#define MU8STR(charLiteralString) (NBT_Type::String(U8TOMU8STR(u8##charLiteralString)))//从工具返回的std::string_view构造到nbt的string::view
#define MU8STRV(charLiteralString) (NBT_Type::String::View(U8TOMU8STR(u8##charLiteralString)))//从工具返回的std::string_view构造到nbt的string::view

template <bool bIsConst>
class NBT_Node_View;

class NBT_Node
{
	template <bool bIsConst>
	friend class NBT_Node_View;

private:
	template <typename T>
	struct TypeListToVariant;

	template <typename... Ts>
	struct TypeListToVariant<NBT_Type::_TypeList<Ts...>>
	{
		using type = std::variant<Ts...>;
	};

	using VariantData = TypeListToVariant<NBT_Type::TypeList>::type;

	VariantData data;
public:
	//显式构造（通过in_place_type_t）
	template <typename T, typename... Args>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && NBT_Type::IsValidType_V<std::decay_t<T>>)
	explicit NBT_Node(std::in_place_type_t<T>, Args&&... args) : data(std::in_place_type<T>, std::forward<Args>(args)...)
	{
		static_assert(std::is_constructible_v<VariantData, Args&&...>, "Invalid constructor arguments for NBT_Node");
	}

	//显式列表构造（通过in_place_type_t）
	template <typename T, typename U, typename... Args>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && NBT_Type::IsValidType_V<std::decay_t<T>>)
	explicit NBT_Node(std::in_place_type_t<T>, std::initializer_list<U> init) : data(std::in_place_type<T>, init)
	{
		static_assert(std::is_constructible_v<VariantData, Args&&...>, "Invalid constructor arguments for NBT_Node");
	}

	//通用构造
	template <typename T>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && NBT_Type::IsValidType_V<std::decay_t<T>>)
	NBT_Node(T &&value) noexcept : data(std::forward<T>(value))
	{
		static_assert(std::is_constructible_v<VariantData, decltype(value)>, "Invalid constructor arguments for NBT_Node");
	}

	//原位放置
	template <typename T, typename... Args>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && NBT_Type::IsValidType_V<std::decay_t<T>>)
	T &emplace(Args&&... args)
	{
		static_assert(std::is_constructible_v<VariantData, Args&&...>, "Invalid constructor arguments for NBT_Node");

		return data.emplace<T>(std::forward<Args>(args)...);
	}

	//通用赋值
	template<typename T>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && NBT_Type::IsValidType_V <std::decay_t<T>>)
	NBT_Node &operator=(T &&value) noexcept
	{
		static_assert(std::is_constructible_v<VariantData, decltype(value)>, "Invalid constructor arguments for NBT_Node");

		data = std::forward<T>(value);
		return *this;
	}

	// 默认构造（TAG_End）
	NBT_Node() : data(NBT_Type::End{})
	{}

	// 自动析构由variant处理
	~NBT_Node() = default;

	NBT_Node(const NBT_Node &_NBT_Node) : data(_NBT_Node.data)
	{}

	NBT_Node(NBT_Node &&_NBT_Node) noexcept : data(std::move(_NBT_Node.data))
	{}

	NBT_Node &operator=(const NBT_Node &_NBT_Node)
	{
		data = _NBT_Node.data;
		return *this;
	}

	NBT_Node &operator=(NBT_Node &&_NBT_Node) noexcept
	{
		data = std::move(_NBT_Node.data);
		return *this;
	}

	bool operator==(const NBT_Node &_Right) const noexcept
	{
		return data == _Right.data;
	}
	
	bool operator!=(const NBT_Node &_Right) const noexcept
	{
		return data != _Right.data;
	}

	std::partial_ordering operator<=>(const NBT_Node &_Right) const noexcept
	{
		return data <=> _Right.data;
	}

	//清除所有数据
	void Clear(void)
	{
		data.emplace<NBT_Type::End>(NBT_Type::End{});
	}

	//获取标签类型
	NBT_TAG GetTag() const noexcept
	{
		return (NBT_TAG)data.index();//返回当前存储类型的index（0基索引，与NBT_TAG enum一一对应）
	}


	//类型安全访问
	template<typename T>
	const T &GetData() const
	{
		return std::get<T>(data);
	}

	template<typename T>
	T &GetData()
	{
		return std::get<T>(data);
	}

	// 类型检查
	template<typename T>
	bool TypeHolds() const
	{
		return std::holds_alternative<T>(data);
	}

	//针对每种类型重载一个方便的函数
	/*
		纯类型名函数：直接获取此类型，不做任何检查，由标准库std::get具体实现决定
		Is开头的类型名函数：判断当前NBT_Node是否为此类型
		纯类型名函数带参数版本：查找当前Compound指定的Name并转换到类型引用返回，不做检查，具体由标准库实现定义
		Has开头的类型名函数带参数版本：查找当前Compound是否有特定Name的Tag，并返回此Name的Tag（转换到指定类型）的指针
	*/
#define TYPE_GET_FUNC(type)\
const NBT_Type::type &Get##type() const\
{\
	return std::get<NBT_Type::type>(data);\
}\
\
NBT_Type::type &Get##type()\
{\
	return std::get<NBT_Type::type>(data);\
}\
\
bool Is##type() const\
{\
	return std::holds_alternative<NBT_Type::type>(data);\
}\
\
friend NBT_Type::type &Get##type(NBT_Node & node)\
{\
	return node.Get##type();\
}\
\
friend const NBT_Type::type &Get##type(const NBT_Node & node)\
{\
	return node.Get##type();\
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

/*
TODO:
给NBTList与NBTCompound添加一个附加字段
往里面插入数据的时候，会根据数据的大小计算
出nbt的大小合并到自身，删除的时候去掉
这样在写出的时候可以预分配，而不是被动的触发
容器的扩容拷贝，增加写出性能
*/