#pragma once

//#include <list>
#include <vector>
#include <map>
#include <variant>
#include <type_traits>
#include <string>
#include <stdint.h>

class NBT_Node
{
public:
	enum NBT_TAG : size_t
	{
		TAG_End = 0,	//������
		TAG_Byte,		//int8_t
		TAG_Short,		//int16_t
		TAG_Int,		//int32_t
		TAG_Long,		//int64_t
		TAG_Float,		//float 4byte
		TAG_Double,		//double 8byte
		TAG_Byte_Array,	//std::vector<int8_t>
		TAG_String,		//std::string->�г������ݣ���Ϊ��0��ֹ�ַ���!!
		TAG_List,		//std::list<NBT_Node>->vector
		TAG_Compound,	//std::map<std::string, NBT_Node>->�ַ���ΪNBT������
		TAG_Int_Array,	//std::vector<int32_t>
		TAG_Long_Array,	//std::vector<int64_t>
		ENUM_END,		//������ǣ����ڼ���enumԪ�ظ���
	};

	using NBT_End			= std::monostate;//��״̬
	using NBT_Byte			= int8_t;
	using NBT_Short			= int16_t;
	using NBT_Int			= int32_t;
	using NBT_Long			= int64_t;
	using NBT_Float			= std::conditional_t<(sizeof(float) == sizeof(uint32_t)), float, uint32_t>;//ͨ��������ȷ�����ʹ�С��ѡ����ȷ�����ͣ����ȸ������ͣ����ʧ�����滻Ϊ��Ӧ�Ŀ�������
	using NBT_Double		= std::conditional_t<(sizeof(double) == sizeof(uint64_t)), double, uint64_t>;
	using NBT_Byte_Array	= std::vector<int8_t>;
	using NBT_Int_Array		= std::vector<int32_t>;
	using NBT_Long_Array	= std::vector<int64_t>;
	using NBT_String		= std::string;
	using NBT_List			= std::vector<NBT_Node>;//�洢һϵ��ͬ���ͱ�ǩ����Ч���أ��ޱ�ǩ ID �����ƣ�//ԭ��Ϊlist����Ϊmc��listҲͨ���±���ʣ���Ϊvectorģ��
	using NBT_Compound		= std::map<std::string, NBT_Node>;//���������µ����ݶ�ͨ��map������

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
		NBT_Byte_Array,
		NBT_String,
		NBT_List,
		NBT_Compound,
		NBT_Int_Array,
		NBT_Long_Array
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

	//enum��������С���
	static_assert((std::variant_size_v<VariantData>) == ENUM_END, "Enumeration does not match the number of types in the mutator");

	// ���ʹ��ڼ��
	template <typename T, typename List>
	struct IsValidType;

	template <typename T, typename... Ts>
	struct IsValidType<T, TypeList<Ts...>>
	{
		static constexpr bool value = (std::is_same_v<T, Ts> || ...);
	};

public:
	// ͨ�ù��캯��
	// ʹ��SFINAE�ų�NBT_Node����
	template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, NBT_Node>>>
	explicit NBT_Node(T &&value) : data(std::forward<T>(value))
	{
		static_assert(IsValidType<std::decay_t<T>, NBT_TypeList>::value, "Invalid type for NBT node");
	}

	// Ĭ�Ϲ��죨TAG_End��
	NBT_Node() : data(std::monostate{})
	{}

	// �Զ�������variant����
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

	//�����������
	void Clear(void)
	{
		data = NBT_Compound{};
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
};
