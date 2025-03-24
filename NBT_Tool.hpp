#pragma once

#include "NBT_Node.hpp"

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

	enum ErrCode : int
	{
		Compound_End = 1,//����
		AllOk = 0,//û������
		InternalTypeError = -1,//����NBT�ڵ����ʹ��󣨴������⣩
		OutOfRangeError = -2,//��NBT�ڲ����ȴ����������NBT�ļ����⣩
		NbtTypeTagError = -3,//NBT��ǩ���ʹ���NBT�ļ����⣩
	};

	static inline const char *const errReason[] =
	{
		"AllOk",
		"InternalTypeError",
		"OutOfRangeError",
		"NbtTypeTagError",
	};

	enum WarnCode : int
	{
		NoWarn = 0,
		ElementExistsWarn = 1,
	};

	static inline const char *const warnReason[] =
	{
		"NoWarn",
		"ElementExistsWarn",
	};

	//ʹ�ñ���βα�+vprintf���������������������չ��Ϣ
	template <typename T, typename std::enable_if<std::is_same<T, ErrCode>::value || std::is_same<T, WarnCode>::value, int>::type = 0>
	static int _cdecl Error(T code, const std::string &data, const size_t &szCurrent, _Printf_format_string_ const char *const cpExtraInfo = NULL, ...)//gccʹ��__attribute__((format))
	{
		if constexpr (std::is_same<T, ErrCode>::value)
		{
			if (code >= AllOk)
			{
				return (int)code;
			}
			//�Ϸ�if��֤errcΪ�����˴���ת���ʱ�֤�����⣨���Ǵ��봫���쳣�����룩
			printf("Read Err[%d]: \"%s\"\n", code, errReason[-(int)code]);
		}
		else if constexpr (std::is_same<T, WarnCode>::value)
		{
			if (code >= NoWarn)
			{
				return (int)code;
			}
			//���warn����
			printf("Read Warn[%d]: \"%s\"\n", code, warnReason[(int)code]);
		}
		else
		{
			static_assert(false, "Unknow [T code] Type!");
		}

		if (cpExtraInfo != NULL)
		{
			printf("Extra Info:\"");
			va_list args;//�䳤�β�
			va_start(args, cpExtraInfo);
			vprintf(cpExtraInfo, args);
			va_end(args);
			printf("\"\n");
		}

		//������ԣ�Ԥ��szCurrentǰ��n���ַ���������е��߽�
#define VIEW_PRE 32//��ǰ
#define VIEW_SUF (32 + 8)//���

		size_t rangeBeg = (szCurrent > VIEW_PRE) ? (szCurrent - VIEW_PRE) : 0;//�ϱ߽����
		size_t rangeEnd = ((szCurrent + VIEW_SUF) < data.size()) ? (szCurrent + VIEW_SUF) : data.size();//�±߽����
		printf("Data Review:\nCurrent: 0x%02llX(%zu)\nData Size: 0x%02llX(%zu)\nData[0x%02llX(%zu)] ~ Data[0x%02llX(%zu)]:\n",
			(uint64_t)szCurrent, szCurrent, (uint64_t)data.size(), data.size(), (uint64_t)rangeBeg, rangeBeg, (uint64_t)rangeEnd, rangeEnd);
		
		for (size_t i = rangeBeg; i < rangeEnd; ++i)
		{
			if ((i - rangeBeg) % 8 == 0)//�����ַ
			{
				if (i != rangeBeg)//��ȥ��һ��ÿ8������
				{
					printf("\n");
				}
				printf("0x%02llX: ", (uint64_t)i);
			}

			if (i != szCurrent)
			{
				printf(" %02X ", (uint8_t)data[i]);
			}
			else//����ǵ�ǰ�����ֽڣ��ӷ����ſ���
			{
				printf("[%02X]", (uint8_t)data[i]);
			}
		}

		if constexpr (std::is_same<T, ErrCode>::value)
		{
			printf("\nSkip err data and clear!\n");
		}
		else if constexpr (std::is_same<T, WarnCode>::value)
		{
			printf("\nSkip warn data and continue...\n");
		}
		else
		{
			static_assert(false, "Unknow [T code] Type!");
		}

		return (int)code;
	}


	static int GetName(const std::string &data, size_t &szCurrent, std::string &sName)
	{
		//��ȡ2�ֽڵ��޷������Ƴ���
		uint16_t wNameLength = 0;//w->word=2*byte
		if (!ReadBigEndian(data, szCurrent, wNameLength))
		{
			return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": wNameLength Read");
		}

		//�жϳ����Ƿ񳬹�
		if (szCurrent + wNameLength >= data.size())
		{
			return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": szCurrent[%zu] + wNameLength[%zu] [%zu]>= data.size()[%zu]",
				szCurrent, wNameLength, szCurrent + (size_t)wNameLength, data.size());
		}

		//����������
		 sName = { data.begin() + szCurrent, data.begin() + (szCurrent + wNameLength) };//�������Ϊ0����0���ַ������Ϸ���Ϊ
		 szCurrent += wNameLength;//�ƶ��±�
		 return AllOk;
	}


	template<typename T, bool bHasName = true>
	static int GetbuiltInType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//��ȡNBT��N�����ƣ�
		std::string sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			int iRet = GetName(data, szCurrent, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//��ȡ����
		if constexpr (std::is_same<T, NBT_Node::NBT_Float>::value)//����������
		{
			uint32_t tTmpData = 0;
			if (!ReadBigEndian(data, szCurrent, tTmpData))
			{
				return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": tTmpData Read");
			}

			if constexpr (bHasName)
			{
				//����-�ں����ݵĽڵ���뵱ǰ����ջ��ȵĸ��ڵ�
				auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), NBT_Node{ (*((NBT_Node::NBT_Float *)&tTmpData)) });//������������ת��
				if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
				{
					Error(ElementExistsWarn, data, szCurrent, __FUNCSIG__ ": the \"%s\"[%s] data already exist!", sName, typeid(NBT_Node::NBT_Float).name());
				}
			}
			else
			{
				//������Ϊ�б�Ԫ�أ�ֱ���޸�nRoot
				nRoot = NBT_Node{ (*((NBT_Node::NBT_Float *)&tTmpData)) };
			}
			
		}
		else if constexpr (std::is_same<T, NBT_Node::NBT_Double>::value)//����������
		{
			uint64_t tTmpData = 0;
			if (!ReadBigEndian(data, szCurrent, tTmpData))
			{
				return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": tTmpData Read");
			}

			if constexpr (bHasName)
			{
				//����-�ں����ݵĽڵ���뵱ǰ����ջ��ȵĸ��ڵ�
				auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), NBT_Node{ (*((NBT_Node::NBT_Double *)&tTmpData)) });//������������ת��
				if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
				{
					Error(ElementExistsWarn, data, szCurrent, __FUNCSIG__ ": the \"%s\"[%s] data already exist!", sName, typeid(NBT_Node::NBT_Double).name());
				}
			}
			else
			{
				//������Ϊ�б�Ԫ�أ�ֱ���޸�nRoot
				nRoot = NBT_Node{ (*((NBT_Node::NBT_Double *)&tTmpData)) };
			}
		}
		else if constexpr (std::is_integral<T>::value)
		{
			T tTmpData = 0;
			if (!ReadBigEndian(data, szCurrent, tTmpData))
			{
				return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": tTmpData Read");
			}

			if constexpr (bHasName)
			{
				//����-�ں����ݵĽڵ���뵱ǰ����ջ��ȵĸ��ڵ�
				auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), NBT_Node{ tTmpData });
				if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
				{
					Error(ElementExistsWarn, data, szCurrent, __FUNCSIG__ ": the \"%s\"[%s] data already exist!", sName, typeid(tTmpData).name());
				}
			}
			else
			{
				nRoot = NBT_Node{ tTmpData };
			}
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

	template<typename T, bool bHasName = true>
	static int GetArrayType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//�ж��ǲ���vector<x>
		if constexpr (!is_std_vector<T>::value)
		{
			static_assert(false, "Not a legal type call!");//�׳��������
		}

		//��ȡNBT��N�����ƣ�
		std::string sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			int iRet = GetName(data, szCurrent, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//��ȡ4�ֽ��з���������������Ԫ�ظ���
		int32_t dwElementCount = 0;//dw->double-word=4*byte
		if (!ReadBigEndian(data, szCurrent, dwElementCount))
		{
			return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": dwElementCount Read");
		}

		//�жϳ����Ƿ񳬹�
		if (szCurrent + dwElementCount * sizeof(T::value_type) >= data.size())//��֤�·����ð�ȫ
		{
			return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": szCurrent[%zu] + dwElementCount[%zu] * sizeof(T::value_type)[%zu] [%zu]>= data.size()[%zu]", 
				szCurrent, (size_t)dwElementCount, sizeof(T::value_type), szCurrent + (size_t)dwElementCount * sizeof(T::value_type), data.size());
		}
		
		//���鱣��
		T tArray;
		tArray.reserve(dwElementCount);//��ǰ����

		//��ȡdElementCount��Ԫ��
		for (int32_t i = 0; i < dwElementCount; ++i)
		{
			typename T::value_type tTmpData;
			FastReadBigEndian(data, szCurrent, tTmpData);//������Ҫȷ����Χ��ȫ
			tArray.emplace_back(tTmpData);//��ȡһ������һ��
		}
		
		if constexpr (bHasName)
		{
			//��ɺ����
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), NBT_Node{ std::move(tArray) });
			if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
			{
				Error(ElementExistsWarn, data, szCurrent, __FUNCSIG__ ": the \"%s\"[%s] data already exist!", sName, typeid(tArray).name());
			}
		}
		else//�����ƣ�Ϊ�б�Ԫ��
		{
			nRoot = NBT_Node{ std::move(tArray) };
		}
		return AllOk;
	}

	template<bool bHasName = true>
	static int GetCompoundType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//��ȡNBT��N�����ƣ�
		std::string sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			int iRet = GetName(data, szCurrent, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//��ʼ�ݹ�
		NBT_Node nodeTemp{ NBT_Node::NBT_Compound{} };
		int iRet = GetNBT(data, szCurrent, nodeTemp);
		if (iRet < AllOk)
		{
			return iRet;
		}

		if constexpr (bHasName)
		{
			//�ݹ���ɣ������ӽڵ��ѵ�λ
			//ȡ��NBT_Compound�ҵ��Լ��������ƶ���
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(sName, NBT_Node{ std::move(nodeTemp.GetData<NBT_Node::NBT_Compound>()) });
			if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
			{
				Error(ElementExistsWarn, data, szCurrent, __FUNCSIG__ ": the \"%s\"[%s] data already exist!", sName, typeid(NBT_Node::NBT_Compound).name());
			}
		}
		else//�����ƣ�Ϊ�б�Ԫ��
		{
			nRoot = NBT_Node{ std::move(nodeTemp.GetData<NBT_Node::NBT_Compound>()) };
		}

		return AllOk;
	}

	template<bool bHasName = true>
	static int GetStringType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//��ȡNBT��N�����ƣ�
		std::string sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			int iRet = GetName(data, szCurrent, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//��ȡ2�ֽڵ��޷������Ƴ���
		uint16_t wStrLength = 0;//w->word=2*byte
		if (!ReadBigEndian(data, szCurrent, wStrLength))
		{
			return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": wStrLength Read");
		}

		//�жϳ����Ƿ񳬹�
		if (szCurrent + wStrLength >= data.size())
		{
			return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": szCurrent[%zu] + wStrLength[%zu] [%zu]>= data.size()[%zu]",
				szCurrent, (size_t)wStrLength, szCurrent + (size_t)wStrLength, data.size());
		}

		if constexpr (bHasName)
		{
			//ԭλ����
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), NBT_Node{ std::string{data.begin() + szCurrent, data.begin() + (szCurrent + wStrLength)} });
			if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
			{
				Error(ElementExistsWarn, data, szCurrent, __FUNCSIG__ ": the \"%s\"[%s] data already exist!", sName, typeid(std::string).name());
			}
		}
		else//�б�Ԫ��ֱ�Ӹ�ֵ
		{
			nRoot = NBT_Node{ std::string{data.begin() + szCurrent, data.begin() + (szCurrent + wStrLength)} };
		}
		szCurrent += wStrLength;//�ƶ��±�
		
		return AllOk;
	}

	template<bool bHasName = true>
	static int GetListType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//��ȡNBT��N�����ƣ�
		std::string sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			int iRet = GetName(data, szCurrent, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//��ȡ1�ֽڵ��б�Ԫ������
		uint8_t bListElementType = 0;//b=byte
		if (!ReadBigEndian(data, szCurrent, bListElementType))
		{
			return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": bListElementType Read");
		}


		//��ȡ4�ֽڵ��з����б���
		int32_t dwListLength = 0;//dw=double-world=4*byte
		if (!ReadBigEndian(data, szCurrent, dwListLength))
		{
			return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": dwListLength Read");
		}

		//����Ԫ�����ͣ���ȡn���б�
		NBT_Node::NBT_List tmpList;
		for (int32_t i = 0; i < dwListLength; ++i)
		{
			NBT_Node tmpNode{};//�б�Ԫ�ػ�ֱ�Ӹ�ֵ�޸�
			int iRet = SwitchNBT<false>(data, szCurrent, tmpNode, (NBT_Node::NBT_TAG)bListElementType);

			if (iRet != AllOk)
			{
				return iRet;
			}

			//ÿ��ȡһ���������һ��
			tmpList.emplace_back(std::move(tmpNode));
		}

		//�б��Ƕ�ף����Դ�����Ƕ���������
		if constexpr (bHasName)
		{
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(sName, NBT_Node{ std::move(tmpList) });
			if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
			{
				Error(ElementExistsWarn, data, szCurrent, __FUNCSIG__ ": the \"%s\"[%s] data already exist!", sName, typeid(tmpList).name());
			}
		}
		else//�б��е��б�ֱ�Ӹ�ֵ���������в���
		{
			nRoot = NBT_Node{ std::move(tmpList) };
		}

		return AllOk;
	}

	template<bool bHasName = true>
	static inline int SwitchNBT(const std::string &data, size_t &szCurrent, NBT_Node &nRoot, NBT_Node::NBT_TAG tag)
	{
		if (szCurrent >= data.size() && tag != NBT_Node::TAG_End)//���tag��ǰ���ǽ�β����ֱ����ȥ�����β����
		{
			return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": szCurrent[%zu] >= data.size()[%zu]", szCurrent, data.size());
		}

		int iRet = AllOk;

		switch (tag)
		{
		case NBT_Node::TAG_End:
			{
				iRet = Compound_End;
			}
			break;
		case NBT_Node::TAG_Byte:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Byte, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Short:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Short, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Int:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Int, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Long:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Long, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Float:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Float, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Double:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Double, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Byte_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Byte_Array, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_String:
			{
				iRet = GetStringType<bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_List://��Ҫ�ݹ���ã��б�ͷ������ǩID�ͳ��ȣ�������Ϊһϵ��ͬ���ͱ�ǩ����Ч���أ��ޱ�ǩ ID �����ƣ�
			{//���
				iRet = GetListType<bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Compound://��Ҫ�ݹ����
			{
				iRet = GetCompoundType<bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Int_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Int_Array, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Long_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Long_Array, bHasName>(data, szCurrent, nRoot);
			}
			break;
		default://NBT�ڱ�����ǩ����
			{
				iRet = Error(NbtTypeTagError, data, szCurrent, __FUNCSIG__ ": NBT Tag switch default: Unknow Type Tag[%02X(%d)]", tag, tag);//�˴���������ǰ���أ�����Ĭ�Ϸ��ش���
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
			return Error(InternalTypeError, data, szCurrent, __FUNCSIG__ ": nRoot is not type: [%s]", typeid(NBT_Node::NBT_Compound).name());
		}

		int iRet;
		do
		{
			iRet = SwitchNBT(data, szCurrent, nRoot, (NBT_Node::NBT_TAG)(uint8_t)data[szCurrent++]);
		} while (iRet == AllOk);
		
		return iRet;
	}
public:
	NBT_Tool(void) = default;
	NBT_Tool(const std::string &data)
	{
		SetNBT(data);
	}
	~NBT_Tool(void) = default;

	bool SetNBT(const std::string &data)//����nbt������
	{//�����û���˵�����ø���
		nRoot.Clear();//���ԭ�������ݣ�ע�����nbt�ϴ������£�����һ������ĵݹ�������̣����ų�ջ�ռ䲻�㵼������ʧ�ܣ�
		size_t szCurrent{ 0 };
		return GetNBT(data, szCurrent, nRoot) == Compound_End;//��������˵�Ǵ��û�����data��ã�get��nbt����
	}

	NBT_Node& GetRoot(void)
	{
		return nRoot;
	}

	const NBT_Node &GetRoot(void) const
	{
		return nRoot;
	}

	void Print(void) const
	{
		PrintSwitch(nRoot, 0);
	}

private:
	void PrintSwitch(const NBT_Node &nRoot, int iLevel) const
	{
		auto tag = nRoot.GetTag();
		switch (tag)
		{
		case NBT_Node::TAG_End:
			{
				printf("[Compound End]");
			}
			break;
		case NBT_Node::TAG_Byte:
			{
				printf("%db", nRoot.GetData<NBT_Node::NBT_Byte>());
			}
			break;
		case NBT_Node::TAG_Short:
			{
				printf("%ds", nRoot.GetData<NBT_Node::NBT_Short>());
			}
			break;
		case NBT_Node::TAG_Int:
			{
				printf("%d", nRoot.GetData<NBT_Node::NBT_Int>());
			}
			break;
		case NBT_Node::TAG_Long:
			{
				printf("%lldl", nRoot.GetData<NBT_Node::NBT_Long>());
			}
			break;
		case NBT_Node::TAG_Float:
			{
				printf("%ff", nRoot.GetData<NBT_Node::NBT_Float>());
			}
			break;
		case NBT_Node::TAG_Double:
			{
				printf("%lff", nRoot.GetData<NBT_Node::NBT_Double>());
			}
			break;
		case NBT_Node::TAG_Byte_Array:
			{
				auto &arr = nRoot.GetData<NBT_Node::NBT_Byte_Array>();
				printf("[B;");
				for (auto &it : arr)
				{
					printf("%d,", it);
				}
				if (arr.size() != 0)
				{
					printf("\b");
				}

				printf("]");
			}
			break;
		case NBT_Node::TAG_String:
			{
				printf("\"%s\"", nRoot.GetData<NBT_Node::NBT_String>().c_str());
			}
			break;
		case NBT_Node::TAG_List:
			{
				auto &list = nRoot.GetData<NBT_Node::NBT_List>();
				printf("[");
				for (auto &it : list)
				{
					PrintSwitch(it, ++iLevel);
					printf(",");
				}

				if (list.size() != 0)
				{
					printf("\b");
				}
				printf("]");
			}
			break;
		case NBT_Node::TAG_Compound:
			{
				auto &cpd = nRoot.GetData<NBT_Node::NBT_Compound>();
				printf("{");

				for (auto &it : cpd)
				{
					printf("\"%s\":", it.first.c_str());
					PrintSwitch(it.second, ++iLevel);
					printf(",");
				}

				if (cpd.size() != 0)
				{
					printf("\b");
				}
				printf("}");
			}
			break;
		case NBT_Node::TAG_Int_Array:
			{
				auto &arr = nRoot.GetData<NBT_Node::NBT_Int_Array>();
				printf("[I;");
				for (auto &it : arr)
				{
					printf("%d,", it);
				}

				if (arr.size() != 0)
				{
					printf("\b");
				}
				printf("]");
			}
			break;
		case NBT_Node::TAG_Long_Array:
			{
				auto &arr = nRoot.GetData<NBT_Node::NBT_Long_Array>();
				printf("[L;");
				for (auto &it : arr)
				{
					printf("%lld,", it);
				}

				if (arr.size() != 0)
				{
					printf("\b");
				}
				printf("]");
			}
			break;
		default:
			{
				printf("[Unknow NBT Tag Type [%02X(%d)]]", tag, tag);
			}
			break;
		}
	}
};