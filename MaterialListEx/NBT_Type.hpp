#pragma once

#include <stdint.h>
#include <variant>
#include <vector>
#include <map>
#include <string>
#include <type_traits>

#include "NBT_TAG.hpp"

class NBT_Node;
template<typename Array>
class MyArray;
template<typename String>
class MyString;
template <typename List>
class MyList;
template<typename Map>
class MyCompound;

class NBT_Type
{
public:
	using End			= std::monostate;//��״̬
	using Byte			= int8_t;
	using Short			= int16_t;
	using Int			= int32_t;
	using Long			= int64_t;
	using Float			= std::conditional_t<(sizeof(float) == sizeof(uint32_t)), float, uint32_t>;//ͨ��������ȷ�����ʹ�С��ѡ����ȷ�����ͣ����ȸ������ͣ����ʧ�����滻Ϊ��Ӧ�Ŀ�������
	using Double		= std::conditional_t<(sizeof(double) == sizeof(uint64_t)), double, uint64_t>;
	using ByteArray		= MyArray<std::vector<Byte>>;
	using IntArray		= MyArray<std::vector<Int>>;
	using LongArray		= MyArray<std::vector<Long>>;
	using String		= MyString<std::string>;//mu8-string
	using List			= MyList<std::vector<NBT_Node>>;//�洢һϵ��ͬ���ͱ�ǩ����Ч���أ��ޱ�ǩ ID �����ƣ�//ԭ��Ϊlist����Ϊmc��listҲͨ���±���ʣ���Ϊvectorģ��
	using Compound		= MyCompound<std::map<String, NBT_Node>>;//���������µ����ݶ�ͨ��map������

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
};