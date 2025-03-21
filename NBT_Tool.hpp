#pragma once
#include <stdint.h>

#include <string>
#include <list>
#include <vector>
#include <map>
#include <variant>

#include <type_traits>

class NBT_Tool;//ǰ������

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
		TAG_List,		//std::list<NBT_Node>
		TAG_Compound,	//std::map<std::string, NBT_Node>->�ַ���ΪNBT������
		TAG_Int_Array,	//std::vector<int32_t>
		TAG_Long_Array,	//std::vector<int64_t>
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
	using NBT_List			= std::list<NBT_Node>;//�洢һϵ��ͬ���ͱ�ǩ����Ч���أ��ޱ�ǩ ID �����ƣ�
	using NBT_Compound		= std::map<std::string, NBT_Node>;//���������µ����ݶ�ͨ��map������

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
	template<typename T>
	explicit NBT_Node(T &&value)
		: tag(deduce_tag<T>()),
		data(std::forward<T>(value))
	{
		static_assert(!std::is_same_v<std::decay_t<T>, NBT_Node>, "Cannot construct NBT_Node from another NBT_Node");
	}

	// Ĭ�Ϲ��죨TAG_End��
	NBT_Node() : tag(TAG_End), data(std::monostate{})
	{}

	// �Զ�������variant����
	~NBT_Node() = default;
	//�ƶ�������variant����
	NBT_Node(NBT_Node &&) = default;
	//����������variant����
	NBT_Node(const NBT_Node &) = default;

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


class NBT_Tool
{
private:
	NBT_Node nRoot{ NBT_Node::NBT_Compound{} };//Ĭ�Ͽսڵ�

	// ��ȡ�������ֵ
	template<typename T>
	static bool ReadBigEndian(const std::string &data, size_t &szCurrent, T& tVal)
	{
		if (szCurrent + sizeof(T) >= data.size())
		{
			return false;
		}

		if constexpr (sizeof(T) == 1)
		{
			tVal = (T)(uint8_t)data[szCurrent++];
		}
		else
		{
			T tTmp = 0;
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				tTmp = (tTmp << 8) | (T)(uint8_t)(data[szCurrent++]);//��Ϊֻ�����ƣ��������з��ŵ��µ�����λ��bug������ת��Ϊ�޷�������
			}
			tVal = tTmp;
		}

		return true;
	}

	// ��ȡ�������ֵ�����ٰ棩����������Ҫȷ��data��Χ��ȫ��
	template<typename T>
	static inline void FastReadBigEndian(const std::string &data, size_t &szCurrent, T &tVal)
	{
		if constexpr (sizeof(T) == 1)
		{
			tVal = (T)(uint8_t)data[szCurrent++];
		}
		else
		{
			T tTmp = 0;
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				tTmp = (tTmp << 8) | (T)(uint8_t)(data[szCurrent++]);//��Ϊֻ�����ƣ��������з��ŵ��µ�����λ��bug������ת��Ϊ�޷�������
			}
			tVal = tTmp;
		}
	}

	enum ErrCode
	{
		AllOk = 0,//û������
		OutOfRange = -1,
		TypeError = -2,
		WhatTheFuck = -1145,//���������ܵĴ���
	};


	static int GetName(const std::string &data, size_t &szCurrent, std::string &sName)
	{
		//��ȡ2�ֽڵ��޷������Ƴ���
		uint16_t wNameLength = 0;//w->word=2*byte
		if (!ReadBigEndian(data, szCurrent, wNameLength))
		{
			return OutOfRange;
		}

		//�жϳ����Ƿ񳬹�
		if (szCurrent + wNameLength >= data.size())
		{
			return OutOfRange;
		}

		//����������
		 sName = { data.begin() + szCurrent, data.begin() + (szCurrent + wNameLength) };//�������Ϊ0����0���ַ������Ϸ���Ϊ
		 return AllOk;
	}


	template<typename T>
	static int GetbuiltInType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//��ȡNBT��N�����ƣ�
		std::string sName{};
		int iRet = GetName(data, szCurrent, sName);
		if (iRet < AllOk)
		{
			return iRet;
		}

		//��ȡ����
		if constexpr (std::is_same<T, NBT_Node::NBT_Float>::value)//����������
		{
			uint32_t tTmpData = 0;
			if (!ReadBigEndian(data, szCurrent, tTmpData))
			{
				return OutOfRange;
			}

			//����-�ں����ݵĽڵ���뵱ǰ����ջ��ȵĸ��ڵ�
			nRoot.GetData<NBT_Node::NBT_Compound>().emplace(std::move(sName), NBT_Node{ (*((NBT_Node::NBT_Float*)&tTmpData)) });//������������ת��
		}
		else if constexpr (std::is_same<T, NBT_Node::NBT_Double>::value)//����������
		{
			uint64_t tTmpData = 0;
			if (!ReadBigEndian(data, szCurrent, tTmpData))
			{
				return OutOfRange;
			}

			//����-�ں����ݵĽڵ���뵱ǰ����ջ��ȵĸ��ڵ�
			nRoot.GetData<NBT_Node::NBT_Compound>().emplace(std::move(sName), NBT_Node{ (*((NBT_Node::NBT_Double *)&tTmpData)) });//������������ת��
		}
		else if constexpr (std::is_integral<T>::value)
		{
			T tTmpData = 0;
			if (!ReadBigEndian(data, szCurrent, tTmpData))
			{
				return OutOfRange;
			}

			//����-�ں����ݵĽڵ���뵱ǰ����ջ��ȵĸ��ڵ�
			nRoot.GetData<NBT_Node::NBT_Compound>().emplace(std::move(sName), NBT_Node{ tTmpData });
		}
		else
		{
			static_assert(false, "Not a legal type call!");//�׳��������
		}

		return AllOk;
	}

	template<typename T>
	struct is_std_vector : std::false_type
	{};

	template<typename T, typename Alloc>
	struct is_std_vector<std::vector<T, Alloc>> : std::true_type
	{};

	template<typename T>
	static int GetArrayType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//��ȡNBT��N�����ƣ�
		std::string sName{};
		int iRet = GetName(data, szCurrent, sName);
		if (iRet < AllOk)
		{
			return iRet;
		}

		//��ȡ4�ֽ��з���������������Ԫ�ظ���
		int32_t dwElementCount = 0;//dw->double-word=4*byte
		if (!ReadBigEndian(data, szCurrent, dwElementCount))
		{
			return OutOfRange;
		}

		//�жϳ����Ƿ񳬹�
		if (szCurrent + dwElementCount * sizeof(T) >= data.size())//��֤�·����ð�ȫ
		{
			return OutOfRange;
		}

		//�ж��ǲ���vector<x>
		if constexpr (!is_std_vector<T>::value)
		{
			static_assert(false, "Not a legal type call!");//�׳��������
		}
		
		//���鱣��
		T tArray;
		tArray.reserve(dwElementCount);//��ǰ����

		//��ȡdElementCount��Ԫ��
		for (int32_t i = 0; i < dwElementCount; ++i)
		{
			typename T::value_type tTmpData;
			FastReadBigEndian(data, szCurrent, tTmpData);//������Ҫȷ����Χ��ȫ
			tArray.push_back(tTmpData);//��ȡһ������һ��
		}
		
		//��ɺ����
		nRoot.GetData<NBT_Node::NBT_Compound>().emplace(std::move(sName), NBT_Node{ std::move(tArray) });
		return AllOk;
	}

	static int GetCompoundType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//��ȡNBT��N�����ƣ�
		std::string sName{};
		int iRet = GetName(data, szCurrent, sName);
		if (iRet < AllOk)
		{
			return iRet;
		}

		//��ʼ�ݹ�
		NBT_Node nodeTemp{ NBT_Node::NBT_Compound{} };
		iRet = GetNBT(data, szCurrent, nodeTemp);
		if (iRet < AllOk)
		{
			return iRet;
		}

		//�ݹ���ɣ������ӽڵ��ѵ�λ
		//ȡ��NBT_Compound�ҵ��Լ��������ƶ���
		nRoot.GetData<NBT_Node::NBT_Compound>().emplace(sName, std::move(nodeTemp.GetData<NBT_Node::NBT_Compound>()));

		return AllOk;
	}

	static int GetStringType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//��ȡNBT��N�����ƣ�
		std::string sName{};
		int iRet = GetName(data, szCurrent, sName);
		if (iRet < AllOk)
		{
			return iRet;
		}

		//��ȡ2�ֽڵ��޷������Ƴ���
		uint16_t wStrLength = 0;//w->word=2*byte
		if (!ReadBigEndian(data, szCurrent, wStrLength))
		{
			return OutOfRange;
		}

		//�жϳ����Ƿ񳬹�
		if (szCurrent + wStrLength >= data.size())
		{
			return OutOfRange;
		}

		//ԭλ����
		nRoot.GetData<NBT_Node::NBT_Compound>().emplace(std::move(sName), NBT_Node{ std::string{data.begin() + szCurrent,data.begin() + (szCurrent + wStrLength)} });
		return AllOk;
	}

	static int GetListType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//��ȡNBT��N�����ƣ�
		std::string sName{};
		int iRet = GetName(data, szCurrent, sName);
		if (iRet < AllOk)
		{
			return iRet;
		}

		//��ȡ1�ֽڵ��б�Ԫ������
		uint8_t bListElementType = 0;//b=byte
		if (!ReadBigEndian(data, szCurrent, bListElementType))
		{
			return OutOfRange;
		}


		//��ȡ4�ֽڵ��з����б���
		int32_t dwListLength = 0;//dw=double-world=4*byte
		if (!ReadBigEndian(data, szCurrent, dwListLength))
		{
			return OutOfRange;
		}

		//����Ԫ�����ͣ���ȡn���б�
		for (int32_t i = 0; i < dwListLength; ++i)
		{
			int iRet = SwitchNBT(data, szCurrent, nRoot, bListElementType);
			if(iRet < AllOk)
			{
				return iRet;
			}



		}
	}


	static inline int SwitchNBT(const std::string &data, size_t &szCurrent, NBT_Node &nRoot, NBT_Node::NBT_TAG tag)
	{
		if (szCurrent >= data.size())
		{
			return OutOfRange;
		}

		int iRet = AllOk;

		switch (tag)
		{
		case NBT_Node::TAG_End:
			{
				return AllOk;
			}
			break;
		case NBT_Node::TAG_Byte:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Byte>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Short:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Short>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Int:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Int>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Long:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Long>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Float:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Float>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Double:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Double>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Byte_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Byte_Array>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_String:
			{
				iRet = GetStringType(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_List://��Ҫ�ݹ���ã��б�ͷ������ǩID�ͳ��ȣ�������Ϊһϵ��ͬ���ͱ�ǩ����Ч���أ��ޱ�ǩ ID �����ƣ�
			{//���

			}
			break;
		case NBT_Node::TAG_Compound://��Ҫ�ݹ����
			{
				iRet = GetCompoundType(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Int_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Int_Array>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Long_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Long_Array>(data, szCurrent, nRoot);
			}
			break;
		default:
			{
				iRet = WhatTheFuck;//�������ô���ֵģ�
			}
			break;
		}

		return iRet;
	}

	static int GetNBT(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)//�ݹ���ö�ȡ����ӽڵ�
	{
		//�ڵ����ͼ�飺��֤��ǰnRoot��NBT_Node::NBT_Compound���ͣ�����ʧ��
		if (!nRoot.TypeHolds<NBT_Node::NBT_Compound>())//���ʹ���
		{
			return TypeError;
		}

		int iRet;
		do
		{
			iRet = SwitchNBT(data, szCurrent, nRoot, data[szCurrent++]);
		} while (iRet == AllOk);
		
	}
public:
	NBT_Tool(void) = default;
	NBT_Tool(const std::string &data)
	{
		GetNBT(data);
	}
	~NBT_Tool(void) = default;

	bool GetNBT(const std::string &data)
	{
		nRoot.Clear();//�������
		size_t szCurrent{ 0 };
		GetNBT(data, szCurrent, nRoot);
	}

	NBT_Node& GetRoot(void)
	{
		return nRoot;
	}

	const NBT_Node &GetRoot(void) const
	{
		return nRoot;
	}
};