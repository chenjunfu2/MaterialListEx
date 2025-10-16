#pragma once

#include <compare>

#include "NBT_TAG.hpp"
#include "NBT_Type.hpp"

#include "NBT_Array.hpp"
#include "NBT_String.hpp"
#include "NBT_List.hpp"
#include "NBT_Compound.hpp"

//在这里，CHAR2MU8STR的定义与NBT_Type::String::View的定义都已完备，给出转换方式以使得所有静态
//字符串初始化到NBT_Type::String::View，方便与标准库重载，而不是直接拿到std::array导致重载失败
//注意任何需要保存MU8STR而不是临时使用MU8STR的情况下，都必须使用NBT_Type::String保存，而不能使用
//NBT_Type::String::View保存，否则msvc不会报错但是使得NBT_Type::String::View持有无效地址导致
//程序崩溃，但是这种情况下gcc和clang都会报错只有msvc不会还能过编译，只能说这是用msvc的福报（大哭）

#define MU8STR(charLiteralString) (NBT_Type::String(U8TOMU8STR(u8##charLiteralString)))//初始化静态字符串为NBT_String
#define MU8STRV(charLiteralString) (NBT_Type::String::View(U8TOMU8STR(u8##charLiteralString)))//初始化静态字符串为NBT_String_View

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