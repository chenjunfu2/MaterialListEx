#pragma once

#include <variant>
#include <stdint.h>
#include <compare>
#include <type_traits>

#include "NBT_TAG.hpp"

#include "NBT_Array.hpp"
#include "NBT_List.hpp"
#include "NBT_String.hpp"
#include "NBT_Compound.hpp"

template <bool bIsConst>
class NBT_Node_View;

class NBT_Node
{
	template <bool bIsConst>
	friend class NBT_Node_View;
public:
	using NBT_End			= std::monostate;//��״̬
	using NBT_Byte			= int8_t;
	using NBT_Short			= int16_t;
	using NBT_Int			= int32_t;
	using NBT_Long			= int64_t;
	using NBT_Float			= std::conditional_t<(sizeof(float) == sizeof(uint32_t)), float, uint32_t>;//ͨ��������ȷ�����ʹ�С��ѡ����ȷ�����ͣ����ȸ������ͣ����ʧ�����滻Ϊ��Ӧ�Ŀ�������
	using NBT_Double		= std::conditional_t<(sizeof(double) == sizeof(uint64_t)), double, uint64_t>;
	using NBT_ByteArray		= MyArray<std::vector<NBT_Byte>>;
	using NBT_IntArray		= MyArray<std::vector<NBT_Int>>;
	using NBT_LongArray		= MyArray<std::vector<NBT_Long>>;
	using NBT_String		= MyString<std::string>;//mu8-string
	using NBT_List			= MyList<std::vector<NBT_Node>>;//�洢һϵ��ͬ���ͱ�ǩ����Ч���أ��ޱ�ǩ ID �����ƣ�//ԭ��Ϊlist����Ϊmc��listҲͨ���±���ʣ���Ϊvectorģ��
	using NBT_Compound		= MyCompound<std::map<NBT_Node::NBT_String, NBT_Node>>;//���������µ����ݶ�ͨ��map������

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
private:
	template <typename T>
	struct TypeListToVariant;

	template <typename... Ts>
	struct TypeListToVariant<TypeList<Ts...>>
	{
		using type = std::variant<Ts...>;
	};

	using VariantData = TypeListToVariant<NBT_TypeList>::type;

	VariantData data;

	//enum��������С���
	static_assert((std::variant_size_v<VariantData>) == NBT_TAG::ENUM_END, "Enumeration does not match the number of types in the mutator");
public:
	// ���ʹ��ڼ��
	template <typename T, typename List>
	struct IsValidType;

	template <typename T, typename... Ts>
	struct IsValidType<T, TypeList<Ts...>>
	{
		static constexpr bool value = (std::is_same_v<T, Ts> || ...);
	};

	// ��ʽ���죨ͨ����ǩ��
	template <typename T, typename... Args>
	explicit NBT_Node(std::in_place_type_t<T>, Args&&... args) : data(std::in_place_type<T>, std::forward<Args>(args)...)
	{
		static_assert(IsValidType<std::decay_t<T>, NBT_TypeList>::value, "Invalid type for NBT node");
	}

	// ��ʽ���죨ͨ����ǩ��
	template <typename T, typename... Args>
	explicit NBT_Node(Args&&... args) : data(std::forward<Args>(args)...)
	{
		static_assert(IsValidType<std::decay_t<T>, NBT_TypeList>::value, "Invalid type for NBT node");
	}

	// ͨ�ù��캯��
	// ʹ��SFINAE�ų�NBT_Node����
	template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, NBT_Node>>>
	explicit NBT_Node(T &&value) : data(std::forward<T>(value))
	{
		static_assert(IsValidType<std::decay_t<T>, NBT_TypeList>::value, "Invalid type for NBT node");
	}

	// ͨ��ԭλ����ӿ�
	template <typename T, typename... Args>
	T &emplace(Args&&... args)
	{
		static_assert(IsValidType<std::decay_t<T>, NBT_TypeList>::value, "Invalid type for NBT node");
		return data.emplace<T>(std::forward<Args>(args)...);
	}

	// Ĭ�Ϲ��죨TAG_End��
	NBT_Node() : data(NBT_End{})
	{}

	// �Զ�������variant����
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

	//�����������
	void Clear(void)
	{
		data.emplace<NBT_End>();
	}

	//��ȡ��ǩ����
	NBT_TAG GetTag() const noexcept
	{
		return (NBT_TAG)data.index();//���ص�ǰ�洢���͵�index��0����������NBT_TAG enumһһ��Ӧ��
	}


	//���Ͱ�ȫ����
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

	// ���ͼ��
	template<typename T>
	bool TypeHolds() const
	{
		return std::holds_alternative<T>(data);
	}

	//���ÿ����������һ������ĺ���
	/*
		��������������ֱ�ӻ�ȡ�����ͣ������κμ�飬�ɱ�׼��std::get����ʵ�־���
		Is��ͷ���������������жϵ�ǰNBT_Node�Ƿ�Ϊ������
		�������������������汾�����ҵ�ǰCompoundָ����Name��ת�����������÷��أ�������飬�����ɱ�׼��ʵ�ֶ���
		Has��ͷ�������������������汾�����ҵ�ǰCompound�Ƿ����ض�Name��Tag�������ش�Name��Tag��ת����ָ�����ͣ���ָ��
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
friend inline NBT_##type &Get##type(NBT_Node & node)\
{\
	return node.Get##type();\
}\
\
friend inline const NBT_##type &Get##type(const NBT_Node & node)\
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