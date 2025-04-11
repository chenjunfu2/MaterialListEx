#pragma once

//#include <list>
#include <vector>
#include <map>
#include <variant>
#include <type_traits>
#include <string>
#include <stdint.h>
#include <typeinfo>

template<typename Map>
class MyCompound :public Map
{
public:
	//继承基类构造
	using Map::Map;

	//简化map查询
	inline Map::mapped_type &Get(const typename Map::key_type &sTagName)
	{
		return Map::at(sTagName);
	}

	inline const Map::mapped_type &Get(const  typename Map::key_type &sTagName) const
	{
		return Map::at(sTagName);
	}

	inline Map::mapped_type *Search(const  typename Map::key_type &sTagName)
	{
		auto find = Map::find(sTagName);
		return find == Map::end() ? NULL : &((*find).second);
	}

	inline const Map::mapped_type *Search(const typename Map::key_type &sTagName) const
	{
		auto find = Map::find(sTagName);
		return find == Map::end() ? NULL : &((*find).second);
	}


#define TYPE_GET_FUNC(type)\
inline const typename Map::mapped_type::NBT_##type &Get##type(const typename Map::key_type & sTagName) const\
{\
	return Map::at(sTagName).Get##type();\
}\
\
inline typename Map::mapped_type::NBT_##type &Get##type(const typename Map::key_type & sTagName)\
{\
	return Map::at(sTagName).Get##type();\
}\
\
inline const typename Map::mapped_type::NBT_##type *Has##type(const typename Map::key_type & sTagName) const\
{\
	auto find = Map::find(sTagName);\
	return find != Map::end() && find->second.Is##type() ? &(find->second.Get##type()) : NULL;\
}\
\
inline typename Map::mapped_type::NBT_##type *Has##type(const typename Map::key_type & sTagName)\
{\
	auto find = Map::find(sTagName);\
	return find != Map::end() && find->second.Is##type() ? &(find->second.Get##type()) : NULL;\
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


class NBT_Node
{
public:
	enum NBT_TAG : uint8_t
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

	using NBT_End			= std::monostate;//无状态
	using NBT_Byte			= int8_t;
	using NBT_Short			= int16_t;
	using NBT_Int			= int32_t;
	using NBT_Long			= int64_t;
	using NBT_Float			= std::conditional_t<(sizeof(float) == sizeof(uint32_t)), float, uint32_t>;//通过编译期确认类型大小来选择正确的类型，优先浮点类型，如果失败则替换为对应的可用类型
	using NBT_Double		= std::conditional_t<(sizeof(double) == sizeof(uint64_t)), double, uint64_t>;
	using NBT_ByteArray		= std::vector<NBT_Byte>;
	using NBT_IntArray		= std::vector<NBT_Int>;
	using NBT_LongArray		= std::vector<NBT_Long>;
	using NBT_String		= std::string;//mu8-string
	using NBT_List			= std::vector<NBT_Node>;//存储一系列同类型标签的有效负载（无标签 ID 或名称）//原先为list，因为mc内list也通过下标访问，改为vector模拟
	using NBT_Compound		= MyCompound<std::map<NBT_Node::NBT_String, NBT_Node>>;//挂在序列下的内容都通过map绑定名称

	template<typename... Ts> struct TypeList{};
	using NBT_TypeList = TypeList
	<
		NBT_End,
		NBT_Byte,
		NBT_Short,
		NBT_Int,
		NBT_Long,
		NBT_Float,
		NBT_Double,
		NBT_ByteArray,
		NBT_String,
		NBT_List,
		NBT_Compound,
		NBT_IntArray,
		NBT_LongArray
	>;

	template <typename T>
	struct TypeListToVariant;

	template <typename... Ts>
	struct TypeListToVariant<TypeList<Ts...>>
	{
		using type = std::variant<Ts...>;
	};

	using VariantData = TypeListToVariant<NBT_TypeList>::type;
private:
	VariantData data;

	//enum与索引大小检查
	static_assert((std::variant_size_v<VariantData>) == ENUM_END, "Enumeration does not match the number of types in the mutator");

	// 类型存在检查
	template <typename T, typename List>
	struct IsValidType;

	template <typename T, typename... Ts>
	struct IsValidType<T, TypeList<Ts...>>
	{
		static constexpr bool value = (std::is_same_v<T, Ts> || ...);
	};

public:
	// 显式构造（通过标签）
	template <typename T, typename... Args>
	explicit NBT_Node(std::in_place_type_t<T>, Args&&... args) : data(std::in_place_type<T>, std::forward<Args>(args)...)
	{
		static_assert(IsValidType<std::decay_t<T>, NBT_TypeList>::value, "Invalid type for NBT node");
	}

	// 显式构造（通过标签）
	template <typename T, typename... Args>
	explicit NBT_Node(Args&&... args) : data(std::forward<Args>(args)...)
	{
		static_assert(IsValidType<std::decay_t<T>, NBT_TypeList>::value, "Invalid type for NBT node");
	}

	// 通用构造函数
	// 使用SFINAE排除NBT_Node类型
	template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, NBT_Node>>>
	explicit NBT_Node(T &&value) : data(std::forward<T>(value))
	{
		static_assert(IsValidType<std::decay_t<T>, NBT_TypeList>::value, "Invalid type for NBT node");
	}

	// 通用原位构造接口
	template <typename T, typename... Args>
	T &emplace(Args&&... args)
	{
		static_assert(IsValidType<std::decay_t<T>, NBT_TypeList>::value, "Invalid type for NBT node");
		return data.emplace<T>(std::forward<Args>(args)...);
	}

	// 默认构造（TAG_End）
	NBT_Node() : data(std::monostate{})
	{}

	// 自动析构由variant处理
	~NBT_Node() = default;

	NBT_Node(const NBT_Node &_NBT_Node) : data(_NBT_Node.data)
	{}

	NBT_Node(NBT_Node &&_NBT_Node) noexcept : data(std::move(_NBT_Node.data))
	{
		_NBT_Node.data = std::monostate{};
	}

	NBT_Node &operator=(const NBT_Node &_NBT_Node)
	{
		data = _NBT_Node.data;
		return *this;
	}


	NBT_Node &operator=(NBT_Node &&_NBT_Node) noexcept
	{
		data = std::move(_NBT_Node.data);
		_NBT_Node.data = std::monostate{};
		return *this;
	}

	//清除所有数据
	void Clear(void)
	{
		data = NBT_Compound{};
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
inline const NBT_##type &Get##type() const\
{\
	return std::get<NBT_##type>(data);\
}\
\
inline NBT_##type &Get##type()\
{\
	return std::get<NBT_##type>(data);\
}\
\
inline bool Is##type() const\
{\
	return std::holds_alternative<NBT_##type>(data);\
}\
\
friend inline NBT_##type&Get##type(NBT_Node & node)\
{\
	return node.Get##type();\
}\
\
friend inline const NBT_##type&Get##type(const NBT_Node & node)\
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
