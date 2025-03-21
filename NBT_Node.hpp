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
	enum NBT_TAG :uint8_t
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
	};

	using NBT_End = std::monostate;//��״̬
	using NBT_Byte = int8_t;
	using NBT_Short = int16_t;
	using NBT_Int = int32_t;
	using NBT_Long = int64_t;
	using NBT_Float = std::conditional_t<(sizeof(float) == sizeof(uint32_t)), float, uint32_t>;//ͨ��������ȷ�����ʹ�С��ѡ����ȷ�����ͣ����ȸ������ͣ����ʧ�����滻Ϊ��Ӧ�Ŀ�������
	using NBT_Double = std::conditional_t<(sizeof(double) == sizeof(uint64_t)), double, uint64_t>;
	using NBT_Byte_Array = std::vector<int8_t>;
	using NBT_Int_Array = std::vector<int32_t>;
	using NBT_Long_Array = std::vector<int64_t>;
	using NBT_String = std::string;
	using NBT_List = std::vector<NBT_Node>;//�洢һϵ��ͬ���ͱ�ǩ����Ч���أ��ޱ�ǩ ID �����ƣ�//ԭ��Ϊlist����Ϊmc��listҲͨ���±���ʣ���Ϊvectorģ��
	using NBT_Compound = std::map<std::string, NBT_Node>;//���������µ����ݶ�ͨ��map������

	template<typename... Ts> struct TypeList
	{};
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
	NBT_TAG tag;
	VariantData data;


	// ������������
	template <typename T, typename List>
	struct TypeIndex;

	template <typename T, typename... Ts>
	struct TypeIndex<T, TypeList<T, Ts...>>
	{
		static constexpr size_t value = 0;
	};

	template <typename T, typename U, typename... Ts>
	struct TypeIndex<T, TypeList<U, Ts...>>
	{
		static constexpr size_t value = 1 + TypeIndex<T, TypeList<Ts...>>::value;
	};

	// ���ʹ��ڼ��
	template <typename T, typename List>
	struct IsValidType;

	template <typename T, typename... Ts>
	struct IsValidType<T, TypeList<Ts...>>
	{
		static constexpr bool value = (std::is_same_v<T, Ts> || ...);
	};


	// �Զ��Ƶ���ǩ����
	template<typename T>
	static constexpr NBT_TAG deduce_tag()
	{
		static_assert(IsValidType<std::decay_t<T>, NBT_TypeList>::value, "Invalid type for NBT node");
		return static_cast<NBT_TAG>(TypeIndex<std::decay_t<T>, NBT_TypeList>::value);
	}
public:
	// ͨ�ù��캯��
	// ʹ��SFINAE�ų�NBT_Node����
	template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, NBT_Node>>>
	explicit NBT_Node(T &&value) : tag(deduce_tag<T>()), data(std::forward<T>(value))
	{
		static_assert(!std::is_same_v<std::decay_t<T>, NBT_Node>, "Cannot construct NBT_Node from another NBT_Node");
	}

	// Ĭ�Ϲ��죨TAG_End��
	NBT_Node() : tag(TAG_End), data(std::monostate{})
	{}

	// �Զ�������variant����
	~NBT_Node() = default;

	NBT_Node(const NBT_Node &_NBT_Node) : tag(_NBT_Node.tag), data(_NBT_Node.data)
	{}

	NBT_Node(NBT_Node &&_NBT_Node) noexcept : tag(_NBT_Node.tag), data(std::move(_NBT_Node.data))
	{
		_NBT_Node.tag = TAG_End;
		_NBT_Node.data = std::monostate{};
	}

	NBT_Node &operator=(const NBT_Node &_NBT_Node)
	{
		tag = _NBT_Node.tag;
		data = _NBT_Node.data;

		return *this;
	}


	NBT_Node &operator=(NBT_Node &&_NBT_Node) noexcept
	{
		tag = _NBT_Node.tag;
		data = std::move(_NBT_Node.data);

		_NBT_Node.tag = TAG_End;
		_NBT_Node.data = std::monostate{};

		return *this;
	}

	//�����������
	void Clear(void)
	{
		tag = TAG_Compound;
		data = NBT_Compound{};
	}

	// ��ȡ��ǩ����
	NBT_TAG GetTag() const noexcept
	{
		return tag;
	}

	// ��ȡ��ǰ�洢���͵�index
	size_t VariantIndex() const noexcept
	{
		return data.index();
	}

	// ���Ͱ�ȫ����
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
