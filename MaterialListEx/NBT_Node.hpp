#pragma once

#include <compare>

#include "NBT_Type.hpp"
#include "NBT_Array.hpp"
#include "NBT_String.hpp"
#include "NBT_List.hpp"
#include "NBT_Compound.hpp"

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
	//��ʽ���죨ͨ��in_place_type_t��
	template <typename T, typename... Args>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && NBT_Type::IsValidType_V<std::decay_t<T>>)
	explicit NBT_Node(std::in_place_type_t<T>, Args&&... args) : data(std::in_place_type<T>, std::forward<Args>(args)...)
	{
		static_assert(std::is_constructible_v<VariantData, Args&&...>, "Invalid constructor arguments for NBT_Node");
	}

	//��ʽ�б��죨ͨ��in_place_type_t��
	template <typename T, typename U, typename... Args>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && NBT_Type::IsValidType_V<std::decay_t<T>>)
	explicit NBT_Node(std::in_place_type_t<T>, std::initializer_list<U> init) : data(std::in_place_type<T>, init)
	{
		static_assert(std::is_constructible_v<VariantData, Args&&...>, "Invalid constructor arguments for NBT_Node");
	}

	//ͨ�ù���
	template <typename T>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && NBT_Type::IsValidType_V<std::decay_t<T>>)
	NBT_Node(T &&value) noexcept : data(std::forward<T>(value))
	{
		static_assert(std::is_constructible_v<VariantData, decltype(value)>, "Invalid constructor arguments for NBT_Node");
	}

	//ԭλ����
	template <typename T, typename... Args>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && NBT_Type::IsValidType_V<std::decay_t<T>>)
	T &emplace(Args&&... args)
	{
		static_assert(std::is_constructible_v<VariantData, Args&&...>, "Invalid constructor arguments for NBT_Node");

		return data.emplace<T>(std::forward<Args>(args)...);
	}

	//ͨ�ø�ֵ
	template<typename T>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && NBT_Type::IsValidType_V <std::decay_t<T>>)
	NBT_Node &operator=(T &&value) noexcept
	{
		static_assert(std::is_constructible_v<VariantData, decltype(value)>, "Invalid constructor arguments for NBT_Node");

		data = std::forward<T>(value);
		return *this;
	}

	// Ĭ�Ϲ��죨TAG_End��
	NBT_Node() : data(NBT_Type::End{})
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
		data.emplace<NBT_Type::End>(NBT_Type::End{});
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
const NBT_Type::##type &Get##type() const\
{\
	return std::get<NBT_Type::##type>(data);\
}\
\
NBT_Type::##type &Get##type()\
{\
	return std::get<NBT_Type::##type>(data);\
}\
\
bool Is##type() const\
{\
	return std::holds_alternative<NBT_Type::##type>(data);\
}\
\
friend NBT_Type::##type &Get##type(NBT_Node & node)\
{\
	return node.Get##type();\
}\
\
friend const NBT_Type::##type &Get##type(const NBT_Node & node)\
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