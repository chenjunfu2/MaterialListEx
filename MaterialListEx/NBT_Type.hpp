#pragma once

#include <stdint.h>
#include <variant>
#include <vector>
#include <unordered_map>
#include <string>
#include <type_traits>

#include "NBT_TAG.hpp"

class NBT_Node;
template <typename Array>
class MyArray;
template <typename String>
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

	using End			= std::monostate;//��״̬
	using Byte			= int8_t;
	using Short			= int16_t;
	using Int			= int32_t;
	using Long			= int64_t;
	using Float			= std::conditional_t<(sizeof(float) == sizeof(Float_Raw)), float, Float_Raw>;//ͨ��������ȷ�����ʹ�С��ѡ����ȷ�����ͣ����ȸ������ͣ����ʧ�����滻Ϊ��Ӧ�Ŀ�������
	using Double		= std::conditional_t<(sizeof(double) == sizeof(Double_Raw)), double, Double_Raw>;
	using ByteArray		= MyArray<std::vector<Byte>>;
	using IntArray		= MyArray<std::vector<Int>>;
	using LongArray		= MyArray<std::vector<Long>>;
	using String		= MyString<std::string>;//Java MUTF-8 String
	using List			= MyList<std::vector<NBT_Node>>;//�洢һϵ��ͬ���ͱ�ǩ����Ч���أ��ޱ�ǩ ID �����ƣ�//ԭ��Ϊlist����Ϊmc��listҲͨ���±���ʣ���Ϊvectorģ��
	using Compound		= MyCompound<std::unordered_map<String, NBT_Node>>;//���������µ����ݶ�ͨ��map������

	//�����б�
	template<typename... Ts> struct _TypeList{};
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
		"IntArray",
		"LongArray",
		"String",
		"List",
		"Compound",
	};
public:
	constexpr static inline const char *GetTypeName(NBT_TAG tag) noexcept//����ʱ�����жϣ�����̬
	{
		if (tag >= NBT_TAG::ENUM_END)
		{
			return "Unknown";
		}

		NBT_TAG_RAW_TYPE tagRaw = (NBT_TAG_RAW_TYPE)tag;
		return cstrTypeName[tagRaw];
	}

	//���ֽṹ��Length����
	using ArrayLength = int32_t;
	using StringLength = uint16_t;
	using ListLength = int32_t;

	constexpr static inline ArrayLength ArrayLength_Max = INT32_MAX;
	constexpr static inline ArrayLength ArrayLength_Min = INT32_MIN;

	constexpr static inline StringLength StringLength_Max = UINT16_MAX;
	constexpr static inline StringLength StringLength_Min = 0;

	constexpr static inline ListLength ListLength_Max = INT32_MAX;
	constexpr static inline ListLength ListLength_Min = INT32_MIN;

	//������������������
	constexpr static inline Byte Byte_Max = INT8_MAX;
	constexpr static inline Byte Byte_Min = INT8_MIN;

	constexpr static inline Short Short_Max = INT16_MAX;
	constexpr static inline Short Short_Min = INT16_MIN;

	constexpr static inline Int Int_Max = INT32_MAX;
	constexpr static inline Int Int_Min = INT32_MIN;

	constexpr static inline Long Long_Max = INT64_MAX;
	constexpr static inline Long Long_Min = INT64_MIN;

	//���ʹ��ڼ��
	template <typename T, typename List>
	struct IsValidType;

	template <typename T, typename... Ts>
	struct IsValidType<T, _TypeList<Ts...>> : std::bool_constant<(std::is_same_v<T, Ts> || ...)>
	{};

	template <typename T>
	static constexpr bool IsValidType_V = IsValidType<T, TypeList>::value;

	//����������ѯ
	template <typename T, typename... Ts>
	static consteval NBT_TAG_RAW_TYPE TypeTagHelper()//consteval�����������ֵ
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

	//�����б��С
	template <typename List>
	struct TypeListSize;

	template <typename... Ts>
	struct TypeListSize<_TypeList<Ts...>>
	{
		static constexpr size_t value = sizeof...(Ts);
	};

	static constexpr size_t TypeListSize_V = TypeListSize<TypeList>::value;

	//enum��������С���
	static_assert(TypeListSize_V == NBT_TAG::ENUM_END, "Enumeration does not match the number of types in the mutator");


	//��NBT_TAG�����͵�ӳ��
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

	//ӳ�両�����������д��raw����
	template<typename T>
	struct BuiltinRawType
	{
		using Type = T;
		static_assert(IsValidType_V<T> && std::is_integral_v<T>, "Not a legal type!");//�׳��������
	};

	template<>
	struct BuiltinRawType<Float>//������ӳ��
	{
		using Type = Float_Raw;
		static_assert(sizeof(Type) == sizeof(Float), "Type size does not match!");
	};

	template<>
	struct BuiltinRawType<Double>//������ӳ��
	{
		using Type = Double_Raw;
		static_assert(sizeof(Type) == sizeof(Double), "Type size does not match!");
	};

	template<typename T>
	using BuiltinRawType_T = typename BuiltinRawType<T>::Type;
};