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
	//�̳л��๹��
	using Map::Map;

	//��map��ѯ
	inline typename Map::mapped_type &Get(const typename Map::key_type &sTagName)
	{
		return Map::at(sTagName);
	}

	inline const typename Map::mapped_type &Get(const  typename Map::key_type &sTagName) const
	{
		return Map::at(sTagName);
	}

	inline typename Map::mapped_type *Search(const  typename Map::key_type &sTagName)
	{
		auto find = Map::find(sTagName);
		return find == Map::end() ? NULL : &((*find).second);
	}

	inline const typename Map::mapped_type *Search(const typename Map::key_type &sTagName) const
	{
		auto find = Map::find(sTagName);
		return find == Map::end() ? NULL : &((*find).second);
	}

	bool operator==(const MyCompound &_Right) const noexcept
	{
		return (const Map &)*this == (const Map &)_Right;
	}

	bool operator!=(const MyCompound &_Right) const noexcept
	{
		return (const Map &)*this != (const Map &)_Right;
	}

	bool operator<(const MyCompound &_Right) const noexcept
	{
		return (const Map &)*this < (const Map &)_Right;
	}

	bool operator>(const MyCompound &_Right) const noexcept
	{
		return (const Map &)*this > (const Map &)_Right;
	}

	auto operator<=>(const MyCompound &_Right) const noexcept
	{
		return (const Map &)*this <=> (const Map &)_Right;
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


template<typename List>
class MyList :public List
{
public:
	//�̳л��๹��
	using List::List;

	//��list��ѯ
	inline typename List::value_type &Get(const typename List::size_type &szPos)
	{
		return List::at(szPos);
	}

	inline const typename List::value_type &Get(const typename List::size_type &szPos) const
	{
		return List::at(szPos);
	}

	bool operator==(const MyList &_Right) const noexcept
	{
		return (const List &)*this == (const List &)_Right;
	}

	bool operator!=(const MyList &_Right) const noexcept
	{
		return (const List &)*this != (const List &)_Right;
	}

	bool operator<(const MyList &_Right) const noexcept
	{
		return (const List &)*this < (const List &)_Right;
	}

	bool operator>(const MyList &_Right) const noexcept
	{
		return (const List &)*this > (const List &)_Right;
	}

	auto operator<=>(const MyList &_Right) const noexcept
	{
		return (const List &)*this <=> (const List &)_Right;
	}

#define TYPE_GET_FUNC(type)\
inline const typename List::value_type::NBT_##type &Get##type(const typename List::size_type &szPos) const\
{\
	return List::at(szPos).Get##type();\
}\
\
inline typename List::value_type::NBT_##type &Get##type(const typename List::size_type &szPos)\
{\
	return List::at(szPos).Get##type();\
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

template <bool bIsConst>
class NBT_Node_View;

class NBT_Node
{
	template <bool bIsConst>
	friend class NBT_Node_View;
public:
	enum NBT_TAG : uint8_t
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
	using NBT_ByteArray		= std::vector<NBT_Byte>;
	using NBT_IntArray		= std::vector<NBT_Int>;
	using NBT_LongArray		= std::vector<NBT_Long>;
	using NBT_String		= std::string;//mu8-string
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
	static_assert((std::variant_size_v<VariantData>) == ENUM_END, "Enumeration does not match the number of types in the mutator");
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
	
	bool operator<(const NBT_Node &_Right) const noexcept
	{
		return data < _Right.data;
	}
	
	bool operator>(const NBT_Node &_Right) const noexcept
	{
		return data > _Right.data;
	}

	auto operator<=>(const NBT_Node &_Right) const noexcept
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


//TODO: NBT_Node_View
//�÷���NBT_Nodeһ�£�����ֻ���й���ʱ��������ָ�룬�Ҳ����ж���
//������ʱ�����٣�������ٺ�ʹ�ó������ٶ����view����Ϊδ���壬�û����и���
//���촫��nullptrָ���ҽ���ʹ�������Ը�

template <bool bIsConst>
class NBT_Node_View
{
	template <bool bIsConst>
	friend class NBT_Node_View;
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
	struct TypeListPointerToVariant<NBT_Node::TypeList<Ts...>>
	{
		using type = std::variant<PtrType<Ts>...>;//չ����ָ������
	};

	using VariantData = TypeListPointerToVariant<NBT_Node::NBT_TypeList>::type;

	VariantData data;
public:
	static inline constexpr bool is_const = bIsConst;

	// ���ʹ��ڼ��
	template <typename T, typename List>
	struct IsValidType;

	template <typename T, typename... Ts>
	struct IsValidType<T, NBT_Node::TypeList<Ts...>>
	{
		static constexpr bool value = (std::is_same_v<T, Ts> || ...);
	};

	// ͨ�ù��캯��
	template <typename T>
	explicit NBT_Node_View(typename AddConstIf<T>::type &value) : data(&value)
	{
		static_assert(IsValidType<std::decay_t<T>, NBT_Node::NBT_TypeList>::value, "Invalid type for NBT node view");
	}

	//��NBT_Node����
	NBT_Node_View(typename AddConstIf<NBT_Node>::type &node)
	{
		std::visit([this](auto &arg)
			{
				this->data = &arg;
			}, node.data);
	}


	// ����ӷ� const ��ͼ��ʽת���� const ��ͼ
	template<typename = std::enable_if_t<bIsConst>>
	NBT_Node_View(const NBT_Node_View<false> &other)
	{
		std::visit([this](auto &arg)
			{
				this->data = arg;
			}, other.data);
	}

private:
	// ����Ĭ�Ϲ��죨TAG_End��
	NBT_Node_View() = default;
public:

	// �Զ�������variant����
	~NBT_Node_View() = default;

	//��������
	NBT_Node_View(const NBT_Node_View & _NBT_Node_View) : data(_NBT_Node_View.data)
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

	//��ȡ��ǩ����
	NBT_Node::NBT_TAG GetTag() const noexcept
	{
		return (NBT_Node::NBT_TAG)data.index();//���ص�ǰ�洢���͵�index��0����������NBT_TAG enumһһ��Ӧ��
	}

	//���Ͱ�ȫ����
	template<typename T>
	const T &GetData() const
	{
		return *std::get<PtrType<T>>(data);
	}

	template<typename T, typename = std::enable_if_t<!bIsConst>>
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
inline const NBT_Node::NBT_##type &Get##type() const\
{\
	return *std::get<PtrType<NBT_Node::NBT_##type>>(data);\
}\
\
template<typename = std::enable_if_t<!bIsConst>>\
inline NBT_Node::NBT_##type &Get##type()\
{\
	return *std::get<PtrType<NBT_Node::NBT_##type>>(data);\
}\
\
inline bool Is##type() const\
{\
	return std::holds_alternative<PtrType<NBT_Node::NBT_##type>>(data);\
}\
friend inline std::conditional_t<bIsConst, const NBT_Node::NBT_##type &, NBT_Node::NBT_##type &> Get##type(NBT_Node_View & node)\
{\
	return node.Get##type();\
}\
\
friend inline const NBT_Node::NBT_##type &Get##type(const NBT_Node_View & node)\
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