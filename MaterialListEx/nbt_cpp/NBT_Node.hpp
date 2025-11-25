#pragma once

#include <compare>

#include "NBT_Type.hpp"

#include "NBT_Array.hpp"
#include "NBT_String.hpp"
#include "NBT_List.hpp"
#include "NBT_Compound.hpp"

/// @file
/// @brief NBT节点类型，支持存储所有NBT类型的变体

//在这里，所有依赖的定义都已完备，给出方便调用获取字符串字面量的的宏定义

/// @def MU8STR(charLiteralString)
/// @brief 从C风格字符串获取M-UTF-8的字符串
/// @param charLiteralString C风格字符串字面量
/// @return 存储C风格字符串转换到存储M-UTF-8的NBT_Type::String对象
#define MU8STR(charLiteralString) (NBT_Type::String(U8TOMU8STR(u8##charLiteralString)))//从工具返回的std::string_view构造到nbt的string::view

/// @def MU8STRV(charLiteralString)
/// @brief 从C风格字符串获取M-UTF-8的字符串视图
/// @param charLiteralString C风格字符串字面量
/// @return 存储C风格字符串转换到存储M-UTF-8的NBT_Type::String::View视图对象
/// @note 视图对象用于减少String拷贝构造开销，内部直接引用字符数组指针，不持有字符串
#define MU8STRV(charLiteralString) (NBT_Type::String::View(U8TOMU8STR(u8##charLiteralString)))//从工具返回的std::string_view构造到nbt的string::view

template <bool bIsConst>
class NBT_Node_View;

/// @class NBT_Node
/// @brief NBT节点，用于存储NBT格式的各种数据类型
/// @note 该类使用std::variant变体来存储不同类型的NBT数据，提供类型安全的访问和操作接口
class NBT_Node
{
	template <bool bIsConst>
	friend class NBT_Node_View;//视图类作为友元，互相访问数据

private:
	//类型列表展开，声明std::variant
	template <typename T>
	struct TypeListToVariant;

	template <typename... Ts>
	struct TypeListToVariant<NBT_Type::_TypeList<Ts...>>
	{
		using type = std::variant<Ts...>;
	};

	using VariantData = TypeListToVariant<NBT_Type::TypeList>::type;

	//数据对象（要求必须持有数据）
	VariantData data;
public:
	/// @brief 显式类型构造函数（通过in_place_type_t指定目标类型）
	/// @tparam T 要构造的数据类型
	/// @tparam Args 变参模板，接受任意个可构造T类型的参数
	/// @param args 用于构造类型T的参数列表
	/// @note 要求类型必须是NBT_Type类型列表中的任意一个，且不是当前NBT_Node类型，同时参数列表必须要能构造目标类型
	template <typename T, typename... Args>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && NBT_Type::IsValidType_V<std::decay_t<T>>)
	explicit NBT_Node(std::in_place_type_t<T>, Args&&... args) : data(std::in_place_type<T>, std::forward<Args>(args)...)
	{
		static_assert(std::is_constructible_v<VariantData, Args&&...>, "Invalid constructor arguments for NBT_Node");
	}

	/// @brief 显式类型列表构造函数（通过in_place_type_t指定目标类型）
	/// @tparam T 要构造的数据类型
	/// @tparam U 用于构造T的初始化列表的元素类型，一般情况下可自动推导
	/// @param init_list 用于构造类型T的初始化列表
	/// @note 要求类型T必须是NBT_Type类型列表中的任意一个，且不是当前NBT_Node类型，同时初始化列表必须要能构造目标类型
	template <typename T, typename U>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && NBT_Type::IsValidType_V<std::decay_t<T>>)
	explicit NBT_Node(std::in_place_type_t<T>, std::initializer_list<U> init_list) : data(std::in_place_type<T>, init_list)
	{
		static_assert(std::is_constructible_v<VariantData, std::initializer_list<U>>, "Invalid constructor arguments for NBT_Node");
	}

	/// @brief 通用类型构造函数，可以拷贝或移动元素到对象内
	/// @tparam T 用于构造的数据类型
	/// @param value 用于构造的数据值
	/// @note 要求类型T必须是NBT_Type类型列表中的任意一个，且不是当前NBT_Node类型，同时参数必须要能构造目标类型
	template <typename T>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && NBT_Type::IsValidType_V<std::decay_t<T>>)
	NBT_Node(T &&value) noexcept : data(std::forward<T>(value))
	{
		static_assert(std::is_constructible_v<VariantData, decltype(value)>, "Invalid constructor arguments for NBT_Node");
	}

	/// @brief 原位放置新对象并替换当前对象
	/// @tparam T 要替换当前变体的数据类型
	/// @tparam Args 变参模板，接受任意个可构造T类型的参数
	/// @param args 用于构造类型T的参数列表
	/// @return 对新构造对象的引用
	/// @note 要求类型T必须是NBT_Type类型列表中的任意一个，且不是当前NBT_Node类型，同时参数必须要能构造目标类型
	template <typename T, typename... Args>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && NBT_Type::IsValidType_V<std::decay_t<T>>)
	T &emplace(Args&&... args)
	{
		static_assert(std::is_constructible_v<VariantData, Args&&...>, "Invalid constructor arguments for NBT_Node");

		return data.emplace<T>(std::forward<Args>(args)...);
	}

	/// @brief 通用赋值运算符，可以拷贝或移动元素
	/// @tparam T 要替换当前变体的数据类型
	/// @param value 要替换当前变体的数据的值
	/// @return 当前对象的引用
	/// @note 要求类型T必须是NBT_Type类型列表中的任意一个，且不是当前NBT_Node类型，同时参数必须要能构造目标类型
	template<typename T>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && NBT_Type::IsValidType_V <std::decay_t<T>>)
	NBT_Node &operator=(T &&value) noexcept
	{
		static_assert(std::is_constructible_v<VariantData, decltype(value)>, "Invalid constructor arguments for NBT_Node");

		data = std::forward<T>(value);
		return *this;
	}

	/// @brief 默认构造函数（构造为TAG_End类型）
	NBT_Node() : data(NBT_Type::End{})
	{}

	/// @brief 默认析构函数
	~NBT_Node() = default;

	/// @brief 拷贝构造函数
	/// @param _NBT_Node 要拷贝的源对象
	NBT_Node(const NBT_Node &_NBT_Node) : data(_NBT_Node.data)
	{}

	/// @brief 移动构造函数
	/// @param _NBT_Node 要移动的源对象
	NBT_Node(NBT_Node &&_NBT_Node) noexcept : data(std::move(_NBT_Node.data))
	{}

	/// @brief 拷贝赋值运算符
	/// @param _NBT_Node 要拷贝的源对象
	/// @return 当前对象的引用
	NBT_Node &operator=(const NBT_Node &_NBT_Node)
	{
		data = _NBT_Node.data;
		return *this;
	}

	/// @brief 移动赋值运算符
	/// @param _NBT_Node 要移动的源对象
	/// @return 当前对象的引用
	NBT_Node &operator=(NBT_Node &&_NBT_Node) noexcept
	{
		data = std::move(_NBT_Node.data);
		return *this;
	}

	/// @brief 相等比较运算符
	/// @param _Right 要比较的右操作数
	/// @return 是否相等
	bool operator==(const NBT_Node &_Right) const noexcept
	{
		return data == _Right.data;
	}
	
	/// @brief 不等比较运算符
	/// @param _Right 要比较的右操作数
	/// @return 是否不相等
	bool operator!=(const NBT_Node &_Right) const noexcept
	{
		return data != _Right.data;
	}

	/// @brief 三路比较运算符
	/// @param _Right 要比较的右操作数
	/// @return 比较结果，通过std::partial_ordering返回
	std::partial_ordering operator<=>(const NBT_Node &_Right) const noexcept
	{
		return data <=> _Right.data;
	}

	/// @brief 清除所有数据，重置为TAG_End类型
	void Clear(void)
	{
		data.emplace<NBT_Type::End>(NBT_Type::End{});
	}

	/// @brief 获取当前存储的NBT类型的枚举值
	/// @return NBT类型枚举值
	NBT_TAG GetTag() const noexcept
	{
		return (NBT_TAG)(NBT_TAG_RAW_TYPE)data.index();//返回当前存储类型的index（0基索引，与NBT_TAG enum一一对应）
	}


	/// @brief 通过指定类型获取当前存储的数据对象
	/// @tparam T 要访问的数据类型
	/// @return 对存储数据的常量引用
	/// @note 如果类型不存在或当前存储的不是指定类型的指针，则抛出异常，具体请参考std::get的说明
	template<typename T>
	const T &GetData() const
	{
		return std::get<T>(data);
	}

	/// @brief 通过指定类型获取当前存储的数据对象
	/// @tparam T 要访问的数据类型
	/// @return 对存储数据的引用
	/// @note 如果类型不存在或当前存储的不是指定类型的指针，则抛出异常，具体请参考std::get的说明
	template<typename T>
	T &GetData()
	{
		return std::get<T>(data);
	}

	/// @brief 类型判断
	/// @tparam T 要判断的数据类型
	/// @return 当前是否存储指定类型的数据
	template<typename T>
	bool TypeHolds() const
	{
		return std::holds_alternative<T>(data);
	}

//针对每种类型生成一个方便的函数
//通过宏定义批量生成

/// @def TYPE_GET_FUNC(type)
/// @brief 不同类型名接口生成宏
/// @note 用户不应该使用此宏（实际上宏已在使用后取消定义），标注仅为消除doxygen警告
#define TYPE_GET_FUNC(type)\
/**
 @brief 获取当前对象中存储的 type 类型的数据
 @return 对指定类型数据的常量引用
 @note 如果类型不存在或当前存储的不是 type 类型，则抛出异常，具体请参考std::get的说明
 */\
const NBT_Type::type &Get##type() const\
{\
	return std::get<NBT_Type::type>(data);\
}\
\
/**
 @brief 获取当前对象中存储的 type 类型的数据
 @return 对指定类型数据的引用
 @note 如果类型不存在或当前存储的不是 type 类型，则抛出异常，具体请参考std::get的说明
 */\
NBT_Type::type &Get##type()\
{\
	return std::get<NBT_Type::type>(data);\
}\
\
/**
 @brief 检查当前对象中是否存储 type 类型的数据
 @return 是否存储 type 类型
 */\
bool Is##type() const\
{\
	return std::holds_alternative<NBT_Type::type>(data);\
}\
\
/**
 @brief 友元函数：从NBT_Node对象中获取 type 类型的数据
 @param node 要从中获取类型的NBT_Node对象
 @return 对 type 类型数据的引用
 @note 如果类型不存在或当前存储的不是 type 类型，则抛出异常，具体请参考std::get的说明
 */\
friend NBT_Type::type &Get##type(NBT_Node & node)\
{\
	return node.Get##type();\
}\
\
/**
 @brief 友元函数：从NBT_Node对象中获取 type 类型的数据
 @param node 要从中获取类型的NBT_Node对象
 @return 对 type 类型数据的常量引用
 @note 如果类型不存在或当前存储的不是 type 类型，则抛出异常，具体请参考std::get的说明
 */\
friend const NBT_Type::type &Get##type(const NBT_Node & node)\
{\
	return node.Get##type();\
}\
\
/**
 @brief 友元函数：检查目标对象中是否存储 type 类型的数据
 @param node 要检查的对象
 @return 对象中是否存储 type 类型
*/\
friend bool Is##type(const NBT_Node &node)\
{\
	return node.Is##type();\
}

	/// @name 针对每种类型提供一个方便使用的函数，由宏批量生成
	/// @brief 具体作用说明：
	/// - Get开头+类型名的函数：直接获取此类型，异常由std::get具体实现决定
	/// - Is开头 + 类型名的函数：判断当前NBT_Node是否为此类型
	/// @{

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

	/// @}

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