#pragma once

#include "NBT_Node.hpp"

//�÷���NBT_Nodeһ�£�����ֻ���й���ʱ��������ָ�룬�Ҳ����ж���
//������ʱ�����٣�������ٺ�ʹ�ó������ٶ����view����Ϊδ���壬�û����и���
//���촫��nullptrָ���ҽ���ʹ�������Ը�

template <bool bIsConst>
class NBT_Node_View
{
	template <bool bIsConst>
	friend class NBT_Node_View;//��Ҫ�����Լ�Ϊ��Ԫ��������ͬģ�����ʵ��֮������໥����
private:
	template <typename T>
	struct AddConstIf
	{
		using type = std::conditional<bIsConst, const T, T>::type;
	};

	template <typename T>
	using PtrType = typename AddConstIf<T>::type *;

	template <typename T>
	struct TypeListPointerToVariant;

	template <typename... Ts>
	struct TypeListPointerToVariant<NBT_Type::_TypeList<Ts...>>
	{
		using type = std::variant<PtrType<Ts>...>;//չ����ָ������
	};

	using VariantData = TypeListPointerToVariant<NBT_Type::TypeList>::type;

	VariantData data;
public:
	static inline constexpr bool is_const = bIsConst;

	// ͨ�ù��캯���������const�����
	template <typename T>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && !bIsConst)
	NBT_Node_View(T &value) : data(&value)
	{
		static_assert(NBT_Type::IsValidType_V<std::decay_t<T>>, "Invalid type for NBT node view");
	}

	// ͨ�ù��캯��
	template <typename T>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node>)
	NBT_Node_View(const T &value) : data(&value)
	{
		static_assert(NBT_Type::IsValidType_V<std::decay_t<T>>, "Invalid type for NBT node view");
	}

	//��NBT_Node���죨�����const�����
	template <typename = void>//requiresռλģ��
	requires(!bIsConst)
	NBT_Node_View(NBT_Node &node)
	{
		std::visit([this](auto &arg)
			{
				this->data = &arg;
			}, node.data);
	}

	//��NBT_Node����
	NBT_Node_View(const NBT_Node &node)
	{
		std::visit([this](auto &arg)
			{
				this->data = &arg;
			}, node.data);
	}

	// ����ӷ� const ��ͼ��ʽת���� const ��ͼ
	template <typename = void>//requiresռλģ��
	requires(bIsConst)
	NBT_Node_View(const NBT_Node_View<false> &other)
	{
		std::visit([this](auto &arg)
			{
				this->data = arg;//�˴�argΪ��constָ��
			}, other.data);
	}

private:
	// ����Ĭ�Ϲ��죨TAG_End��
	NBT_Node_View() = default;
public:

	// �Զ�������variant����
	~NBT_Node_View() = default;

	//��������
	NBT_Node_View(const NBT_Node_View &_NBT_Node_View) : data(_NBT_Node_View.data)
	{}

	//�ƶ�����
	NBT_Node_View(NBT_Node_View &&_NBT_Node_View) noexcept : data(std::move(_NBT_Node_View.data))
	{}

	NBT_Node_View &operator=(const NBT_Node_View &_NBT_Node_View)
	{
		data = _NBT_Node_View.data;
		return *this;
	}

	NBT_Node_View &operator=(NBT_Node_View &&_NBT_Node_View) noexcept
	{
		data = std::move(_NBT_Node_View.data);
		return *this;
	}


	bool operator==(const NBT_Node_View &_Right) const noexcept
	{
		if (GetTag() != _Right.GetTag())
		{
			return false;
		}

		return std::visit([this](const auto *argL, const auto *argR)-> bool
			{
				using TL = const std::decay_t<decltype(argL)>;
				return *argL == *(TL)argR;
			}, this->data, _Right.data);
	}

	bool operator!=(const NBT_Node_View &_Right) const noexcept
	{
		if (GetTag() != _Right.GetTag())
		{
			return true;
		}

		return std::visit([this](const auto *argL, const auto *argR)-> bool
			{
				using TL = const std::decay_t<decltype(argL)>;
				return *argL != *(TL)argR;
			}, this->data, _Right.data);
	}

	std::partial_ordering operator<=>(const NBT_Node_View &_Right) const noexcept
	{
		if (GetTag() != _Right.GetTag())
		{
			return std::partial_ordering::unordered;
		}

		return std::visit([this](const auto *argL, const auto *argR)-> std::partial_ordering
			{
				using TL = const std::decay_t<decltype(argL)>;
				return *argL <=> *(TL)argR;
			}, this->data, _Right.data);
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
		return *std::get<PtrType<T>>(data);
	}

	template<typename T>
	requires(!bIsConst)
	T &GetData()
	{
		return *std::get<PtrType<T>>(data);
	}

	// ���ͼ��
	template<typename T>
	bool TypeHolds() const
	{
		return std::holds_alternative<PtrType<T>>(data);
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
	return *std::get<PtrType<NBT_Type::##type>>(data);\
}\
\
template <typename = void>\
requires(!bIsConst)\
NBT_Type::##type &Get##type()\
{\
	return *std::get<PtrType<NBT_Type::##type>>(data);\
}\
\
bool Is##type() const\
{\
	return std::holds_alternative<PtrType<NBT_Type::##type>>(data);\
}\
friend std::conditional_t<bIsConst, const NBT_Type::##type &, NBT_Type::##type &> Get##type(NBT_Node_View & node)\
{\
	return node.Get##type();\
}\
\
friend const NBT_Type::##type &Get##type(const NBT_Node_View & node)\
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