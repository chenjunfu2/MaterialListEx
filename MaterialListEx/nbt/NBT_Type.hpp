#pragma once

#include <stdint.h>
#include <stddef.h>//size_t
#include <variant>
#include <vector>
#include <unordered_map>
#include <string>
#include <type_traits>

#include "NBT_TAG.hpp"

class NBT_Node;
template <typename Array>
class MyArray;
template <typename String, typename StringView>
class MyString;
template <typename List>
class MyList;
template <typename Compound>
class MyCompound;

class NBT_Type
{
	NBT_Type(void) = delete;
	~NBT_Type(void) = delete;

public:
	using Float_Raw = uint32_t;
	using Double_Raw = uint64_t;

	using End			= std::monostate;//无状态
	using Byte			= int8_t;
	using Short			= int16_t;
	using Int			= int32_t;
	using Long			= int64_t;
	using Float			= std::conditional_t<(sizeof(float) == sizeof(Float_Raw)), float, Float_Raw>;//通过编译期确认类型大小来选择正确的类型，优先浮点类型，如果失败则替换为对应的可用类型
	using Double		= std::conditional_t<(sizeof(double) == sizeof(Double_Raw)), double, Double_Raw>;
	using ByteArray		= MyArray<std::vector<Byte>>;
	using IntArray		= MyArray<std::vector<Int>>;
	using LongArray		= MyArray<std::vector<Long>>;
	using String		= MyString<std::basic_string<uint8_t>, std::basic_string_view<uint8_t>>;//Java MUTF-8 String
	using List			= MyList<std::vector<NBT_Node>>;//存储一系列同类型标签的有效负载（无标签 ID 或名称）//原先为list，因为mc内list也通过下标访问，改为vector模拟
	using Compound		= MyCompound<std::unordered_map<String, NBT_Node>>;//挂在序列下的内容都通过map绑定名称

	//类型列表
	template<typename... Ts>
	struct _TypeList{};

	//定义
	using TypeList = _TypeList
	<
		End,
		Byte,
		Short,
		Int,
		Long,
		Float,
		Double,
		ByteArray,
		String,
		List,
		Compound,
		IntArray,
		LongArray
	>;

private:
	constexpr static inline const char *const cstrTypeName[] =
	{
		"End",
		"Byte",
		"Short",
		"Int",
		"Long",
		"Float",
		"Double",
		"ByteArray",
		"String",
		"List",
		"Compound",
		"IntArray",
		"LongArray",
	};

public:
	constexpr static inline const char *GetTypeName(NBT_TAG tag) noexcept//运行时类型判断，允许静态
	{
		if (tag >= NBT_TAG::ENUM_END)
		{
			return "Unknown";
		}

		NBT_TAG_RAW_TYPE tagRaw = (NBT_TAG_RAW_TYPE)tag;
		return cstrTypeName[tagRaw];
	}

	//部分结构的Length类型
	using ArrayLength = int32_t;
	using StringLength = uint16_t;
	using ListLength = int32_t;

	constexpr static inline ArrayLength ArrayLength_Max = INT32_MAX;
	constexpr static inline ArrayLength ArrayLength_Min = INT32_MIN;

	constexpr static inline StringLength StringLength_Max = UINT16_MAX;
	constexpr static inline StringLength StringLength_Min = 0;

	constexpr static inline ListLength ListLength_Max = INT32_MAX;
	constexpr static inline ListLength ListLength_Min = INT32_MIN;

	//内置整数类型上下限
	constexpr static inline Byte Byte_Max = INT8_MAX;
	constexpr static inline Byte Byte_Min = INT8_MIN;

	constexpr static inline Short Short_Max = INT16_MAX;
	constexpr static inline Short Short_Min = INT16_MIN;

	constexpr static inline Int Int_Max = INT32_MAX;
	constexpr static inline Int Int_Min = INT32_MIN;

	constexpr static inline Long Long_Max = INT64_MAX;
	constexpr static inline Long Long_Min = INT64_MIN;

	//类型存在检查
	template <typename T, typename List>
	struct IsValidType;

	template <typename T, typename... Ts>
	struct IsValidType<T, _TypeList<Ts...>> : std::bool_constant<(std::is_same_v<T, Ts> || ...)>
	{};

	template <typename T>
	static constexpr bool IsValidType_V = IsValidType<T, TypeList>::value;

	//类型索引查询
	template <typename T, typename... Ts>
	static consteval NBT_TAG_RAW_TYPE TypeTagHelper()//consteval必须编译期求值
	{
		NBT_TAG_RAW_TYPE tagIndex = 0;
		bool bFound = ((std::is_same_v<T, Ts> ? true : (++tagIndex, false)) || ...);
		return bFound ? tagIndex : (NBT_TAG_RAW_TYPE)-1;
	}

	template <typename T, typename List>
	struct TypeTagImpl;

	template <typename T, typename... Ts>
	struct TypeTagImpl<T, _TypeList<Ts...>>
	{
		static constexpr NBT_TAG_RAW_TYPE value = TypeTagHelper<T, Ts...>();
	};

	template <typename T>
	static constexpr NBT_TAG TypeTag_V = (NBT_TAG)TypeTagImpl<T, TypeList>::value;

	//类型列表大小
	template <typename List>
	struct TypeListSize;

	template <typename... Ts>
	struct TypeListSize<_TypeList<Ts...>>
	{
		static constexpr size_t value = sizeof...(Ts);
	};

	static constexpr size_t TypeListSize_V = TypeListSize<TypeList>::value;

	//enum与索引大小检查
	static_assert(TypeListSize_V == NBT_TAG::ENUM_END, "Enumeration does not match the number of types in the mutator");


	//从NBT_TAG到类型的映射
	template <NBT_TAG Tag>
	struct TagToType;

	template <NBT_TAG_RAW_TYPE I, typename List> struct TypeAt;

	template <NBT_TAG_RAW_TYPE I, typename... Ts>
	struct TypeAt<I, _TypeList<Ts...>>
	{
		using type = std::tuple_element_t<I, std::tuple<Ts...>>;
	};

	template <NBT_TAG Tag>
	struct TagToType
	{
		static_assert((NBT_TAG_RAW_TYPE)Tag < TypeListSize_V, "Invalid NBT_TAG");
		using type = typename TypeAt<(NBT_TAG_RAW_TYPE)Tag, TypeList>::type;
	};

	template <NBT_TAG Tag>
	using TagToType_T = typename TagToType<Tag>::type;

	//映射浮点数到方便读写的raw类型
	template<typename T>
	struct BuiltinRawType
	{
		using Type = T;
		static_assert(IsValidType_V<T> && std::is_integral_v<T>, "Not a legal type!");//抛出编译错误
	};

	template<>
	struct BuiltinRawType<Float>//浮点数映射
	{
		using Type = Float_Raw;
		static_assert(sizeof(Type) == sizeof(Float), "Type size does not match!");
	};

	template<>
	struct BuiltinRawType<Double>//浮点数映射
	{
		using Type = Double_Raw;
		static_assert(sizeof(Type) == sizeof(Double), "Type size does not match!");
	};

	template<typename T>
	using BuiltinRawType_T = typename BuiltinRawType<T>::Type;
};