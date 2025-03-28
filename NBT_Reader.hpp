#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"

class InputStream
{
private:
	const std::string &sString;
	size_t szIndex;
public:
	InputStream(const std::string &_sString, size_t szStartIdx = 0) :sString(_sString), szIndex(szStartIdx)
	{}
	~InputStream() = default;

	char GetChar()
	{
		return sString[szIndex++];
	}

	void UnGet()
	{
		if (szIndex != 0)
		{
			--szIndex;
		}
	}

	const char &operator[](size_t szIndex) const
	{
		return sString[szIndex];
	}

	std::string::const_iterator Current() const
	{
		return sString.begin() + szIndex;
	}

	std::string::const_iterator Next(size_t szSize) const
	{
		return sString.begin() + (szIndex + szSize);
	}

	size_t AddIndex(size_t szSize)
	{
		return szIndex += szSize;
	}

	size_t SubIndex(size_t szSize)
	{
		return szIndex -= szSize;
	}

	bool IsEnd() const
	{
		return szIndex >= sString.size();
	}

	size_t Size() const
	{
		return sString.size();
	}

	bool HasAvailData(size_t szSize) const
	{
		return (sString.size() - szIndex) >= szSize;
	}

	void Reset()
	{
		szIndex = 0;
	}

	const std::string &Data() const
	{
		return sString;
	}

	size_t Index() const
	{
		return szIndex;
	}

	size_t &Index()
	{
		return szIndex;
	}
};


class NBT_Reader
{
private:
	// ��ȡ�������ֵ��bNoCheckΪtrue�򲻽����κμ��
	template<bool bNoCheck = false, typename T>
	static std::conditional_t<bNoCheck, void, bool> ReadBigEndian(InputStream &sData, T& tVal)
	{
		if constexpr (!bNoCheck)
		{
			if (!sData.HasAvailData(sizeof(T)))
			{
				return false;
			}
		}

		if constexpr (sizeof(T) == 1)
		{
			tVal = (T)(uint8_t)sData.GetChar();
		}
		else
		{
			T tTmp = 0;
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				tTmp = (tTmp << 8) | (T)(uint8_t)sData.GetChar();//��Ϊֻ�����ƣ��������з��ŵ��µ�����λ��bug������ת��Ϊ�޷�������
			}
			tVal = tTmp;
		}

		if constexpr (!bNoCheck)
		{
			return true;
		}
	}

	enum ErrCode : int
	{
		Compound_End = 1,//����
		AllOk = 0,//û������
		InternalTypeError = -1,//����NBT�ڵ����ʹ��󣨴������⣩
		OutOfRangeError = -2,//��NBT�ڲ����ȴ����������NBT�ļ����⣩
		NbtTypeTagError = -3,//NBT��ǩ���ʹ���NBT�ļ����⣩
		StackDepthExceeded = -4,//����ջ��ȹ��NBT�ļ�or�����������⣩
		ERRCODE_END = -5,//������ǣ�ͳ�Ƹ������ִ�С
	};

	static inline const char *const errReason[] =
	{
		"AllOk",
		"InternalTypeError",
		"OutOfRangeError",
		"NbtTypeTagError",
		"StackDepthExceeded",
	};

	//�ǵ�ͬ�����飡
	static_assert(sizeof(errReason) / sizeof(errReason[0]) == (-ERRCODE_END), "errReason array out sync");

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
	static int _cdecl Error(T code, const InputStream &sData, _Printf_format_string_ const char *const cpExtraInfo = NULL, ...)//gccʹ��__attribute__((format))
	{
		if constexpr (std::is_same<T, ErrCode>::value)
		{
			if (code >= AllOk)
			{
				return (int)code;
			}
			//�Ϸ�if��֤errcΪ�����˴���ת���ʱ�֤�����⣨���Ǵ��봫���쳣�����룩
			printf("Read Err[%d]: \"%s\"\n", (int)code, errReason[-(int)code]);
		}
		else if constexpr (std::is_same<T, WarnCode>::value)
		{
			if (code >= NoWarn)
			{
				return (int)code;
			}
			//���warn����
			printf("Read Warn[%d]: \"%s\"\n", (int)code, warnReason[(int)code]);
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

		size_t rangeBeg = (sData.Index() > VIEW_PRE) ? (sData.Index() - VIEW_PRE) : 0;//�ϱ߽����
		size_t rangeEnd = ((sData.Index() + VIEW_SUF) < sData.Size()) ? (sData.Index() + VIEW_SUF) : sData.Size();//�±߽����
		printf("Data Review:\nCurrent: 0x%02llX(%zu)\nData Size: 0x%02llX(%zu)\nData[0x%02llX(%zu)] ~ Data[0x%02llX(%zu)]:\n",
			(uint64_t)sData.Index(), sData.Index(), (uint64_t)sData.Size(), sData.Size(), (uint64_t)rangeBeg, rangeBeg, (uint64_t)rangeEnd - 1, rangeEnd - 1);
		
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

			if (i != sData.Index())
			{
				printf(" %02X ", (uint8_t)sData[i]);
			}
			else//����ǵ�ǰ�����ֽڣ��ӷ����ſ���
			{
				printf("[%02X]", (uint8_t)sData[i]);
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

#define _RP___FUNCSIG__ __FUNCSIG__//���ڱ�����̶����滻�ﵽ�����ڲ�

#define CHECK_STACK_DEPTH(Depth) \
{\
	if((Depth) <= 0)\
	{\
		return Error(StackDepthExceeded, sData, _RP___FUNCSIG__ ": NBT nesting depth exceeded maximum call stack limit");\
	}\
}

	static int GetName(InputStream &sData, std::string &sName)
	{
		//��ȡ2�ֽڵ��޷������Ƴ���
		uint16_t wNameLength = 0;//w->word=2*byte
		if (!ReadBigEndian(sData, wNameLength))
		{
			return Error(OutOfRangeError, sData, __FUNCSIG__ ": wNameLength Read");
		}

		//�жϳ����Ƿ񳬹�
		if (!sData.HasAvailData(wNameLength))
		{
			return Error(OutOfRangeError, sData, __FUNCSIG__ ": (Index[%zu] + wNameLength[%zu])[%zu] > DataSize[%zu]",
				sData.Index(), (size_t)wNameLength, sData.Index() + (size_t)wNameLength, sData.Size());
		}

		//����������
		sName = { sData.Current(), sData.Next(wNameLength) };//�������Ϊ0����0���ַ������Ϸ���Ϊ
		sData.AddIndex(wNameLength);//�ƶ��±�
		return AllOk;
	}


	template<typename T, bool bHasName = true>
	static int GetbuiltInType(InputStream &sData, NBT_Node &nRoot)
	{
		//��ȡNBT��N�����ƣ�
		std::string sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			int iRet = GetName(sData, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//��ȡ����
		if constexpr (std::is_same<T, NBT_Node::NBT_Float>::value)//����������
		{
			uint32_t tTmpData = 0;
			if (!ReadBigEndian(sData, tTmpData))
			{
				return Error(OutOfRangeError, sData, __FUNCSIG__ ": tTmpData Read");
			}

			if constexpr (bHasName)
			{
				//����-�ں����ݵĽڵ���뵱ǰ����ջ��ȵĸ��ڵ�
				auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), NBT_Node{ (*((NBT_Node::NBT_Float *)&tTmpData)) });//������������ת��
				if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
				{
					Error(ElementExistsWarn, sData, __FUNCSIG__ ": the \"%s\"[%s] sData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(NBT_Node::NBT_Float).name());
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
			if (!ReadBigEndian(sData, tTmpData))
			{
				return Error(OutOfRangeError, sData, __FUNCSIG__ ": tTmpData Read");
			}

			if constexpr (bHasName)
			{
				//����-�ں����ݵĽڵ���뵱ǰ����ջ��ȵĸ��ڵ�
				auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), NBT_Node{ (*((NBT_Node::NBT_Double *)&tTmpData)) });//������������ת��
				if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
				{
					Error(ElementExistsWarn, sData, __FUNCSIG__ ": the \"%s\"[%s] sData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(NBT_Node::NBT_Double).name());
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
			if (!ReadBigEndian(sData, tTmpData))
			{
				return Error(OutOfRangeError, sData, __FUNCSIG__ ": tTmpData Read");
			}

			if constexpr (bHasName)
			{
				//����-�ں����ݵĽڵ���뵱ǰ����ջ��ȵĸ��ڵ�
				auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), NBT_Node{ tTmpData });
				if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
				{
					Error(ElementExistsWarn, sData, __FUNCSIG__ ": the \"%s\"[%s] sData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(tTmpData).name());
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
	static int GetArrayType(InputStream &sData, NBT_Node &nRoot)
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
			int iRet = GetName(sData, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//��ȡ4�ֽ��з���������������Ԫ�ظ���
		int32_t dwElementCount = 0;//dw->double-word=4*byte
		if (!ReadBigEndian(sData, dwElementCount))
		{
			return Error(OutOfRangeError, sData, __FUNCSIG__ ": dwElementCount Read");
		}

		//�жϳ����Ƿ񳬹�
		if (!sData.HasAvailData(dwElementCount * sizeof(T::value_type)))//��֤�·����ð�ȫ
		{
			return Error(OutOfRangeError, sData, __FUNCSIG__ ": (Index[%zu] + dwElementCount[%zu] * sizeof(T::value_type)[%zu])[%zu] > DataSize[%zu]", 
				sData.Index(), (size_t)dwElementCount, sizeof(T::value_type), sData.Index() + (size_t)dwElementCount * sizeof(T::value_type), sData.Size());
		}
		
		//���鱣��
		T tArray;
		tArray.reserve(dwElementCount);//��ǰ����

		//��ȡdElementCount��Ԫ��
		for (int32_t i = 0; i < dwElementCount; ++i)
		{
			typename T::value_type tTmpData;
			ReadBigEndian<true>(sData, tTmpData);//������Ҫȷ����Χ��ȫ
			tArray.emplace_back(tTmpData);//��ȡһ������һ��
		}
		
		if constexpr (bHasName)
		{
			//��ɺ����
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), NBT_Node{ std::move(tArray) });
			if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
			{
				Error(ElementExistsWarn, sData, __FUNCSIG__ ": the \"%s\"[%s] sData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(tArray).name());
			}
		}
		else//�����ƣ�Ϊ�б�Ԫ��
		{
			nRoot = NBT_Node{ std::move(tArray) };
		}
		return AllOk;
	}

	template<bool bHasName = true>
	static int GetCompoundType(InputStream &sData, NBT_Node &nRoot, size_t szStackDepth)//�˵ݹ麯����Ҫ��֤���ݷ���ֵ
	{
		CHECK_STACK_DEPTH(szStackDepth);
		//��ȡNBT��N�����ƣ�
		std::string sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			int iRet = GetName(sData, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//��ʼ�ݹ�
		NBT_Node nodeTemp{ NBT_Node::NBT_Compound{} };
		int iRet = GetNBT(sData, nodeTemp, szStackDepth);//�˴�ֱ�Ӵ��ݣ���ΪGetNBT�ڲ�����еݼ�
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
				Error(ElementExistsWarn, sData, __FUNCSIG__ ": the \"%s\"[%s] sData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(NBT_Node::NBT_Compound).name());
			}
		}
		else//�����ƣ�Ϊ�б�Ԫ��
		{
			nRoot = NBT_Node{ std::move(nodeTemp.GetData<NBT_Node::NBT_Compound>()) };
		}

		return AllOk;
	}

	template<bool bHasName = true>
	static int GetStringType(InputStream &sData, NBT_Node &nRoot)
	{
		//��ȡNBT��N�����ƣ�
		std::string sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			int iRet = GetName(sData, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//��ȡ2�ֽڵ��޷������Ƴ���
		uint16_t wStrLength = 0;//w->word=2*byte
		if (!ReadBigEndian(sData, wStrLength))
		{
			return Error(OutOfRangeError, sData, __FUNCSIG__ ": wStrLength Read");
		}

		//�жϳ����Ƿ񳬹�
		if (!sData.HasAvailData(wStrLength))
		{
			return Error(OutOfRangeError, sData, __FUNCSIG__ ": (Index[%zu] + wStrLength[%zu])[%zu] > DataSize[%zu]",
				sData.Index(), (size_t)wStrLength, sData.Index() + (size_t)wStrLength, sData.Size());
		}

		if constexpr (bHasName)
		{
			//ԭλ����
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), NBT_Node{ std::string{sData.Current(), sData.Next(wStrLength)} });
			if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
			{
				Error(ElementExistsWarn, sData, __FUNCSIG__ ": the \"%s\"[%s] sData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(std::string).name());
			}
		}
		else//�б�Ԫ��ֱ�Ӹ�ֵ
		{
			nRoot = NBT_Node{ std::string{sData.Current(), sData.Next(wStrLength)} };
		}
		sData.AddIndex(wStrLength);//�ƶ��±�
		
		return AllOk;
	}

	template<bool bHasName = true>
	static int GetListType(InputStream &sData, NBT_Node &nRoot, size_t szStackDepth)
	{
		CHECK_STACK_DEPTH(szStackDepth);
		//��ȡNBT��N�����ƣ�
		std::string sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			int iRet = GetName(sData, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//��ȡ1�ֽڵ��б�Ԫ������
		uint8_t bListElementType = 0;//b=byte
		if (!ReadBigEndian(sData, bListElementType))
		{
			return Error(OutOfRangeError, sData, __FUNCSIG__ ": bListElementType Read");
		}

		//������б�Ԫ������
		if (bListElementType >= NBT_Node::NBT_TAG::ENUM_END)
		{
			return Error(NbtTypeTagError, sData, __FUNCSIG__ ": List NBT Type:Unknow Type Tag[%02X(%d)]", bListElementType, bListElementType);
		}

		//��ȡ4�ֽڵ��з����б���
		int32_t dwListLength = 0;//dw=double-world=4*byte
		if (!ReadBigEndian(sData, dwListLength))
		{
			return Error(OutOfRangeError, sData, __FUNCSIG__ ": dwListLength Read");
		}

		//����Ԫ�����ͣ���ȡn���б�
		NBT_Node::NBT_List tmpList;
		for (int32_t i = 0; i < dwListLength; ++i)
		{
			NBT_Node tmpNode{};//�б�Ԫ�ػ�ֱ�Ӹ�ֵ�޸�
			int iRet = SwitchNBT<false>(sData, tmpNode, (NBT_Node::NBT_TAG)bListElementType, szStackDepth - 1);

			if (iRet != AllOk)//���iRet��Compound_End���б����n��NBT_Node��ֵ��NBT_NodeĬ�ϳ�ʼ����ΪTAG_End�������߼�
			{
				return iRet;//�˴�ֻ��ʧ��ʱ���ݷ���ֵ
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
				Error(ElementExistsWarn, sData, __FUNCSIG__ ": the \"%s\"[%s] sData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(tmpList).name());
			}
		}
		else//�б��е��б�ֱ�Ӹ�ֵ���������в���
		{
			nRoot = NBT_Node{ std::move(tmpList) };
		}

		return AllOk;//�б�ͬʱ��ΪԪ�أ��ɹ�Ӧ�÷���Ok�������Ǵ��ݷ���ֵ
	}

	template<bool bHasName = true>
	static int SwitchNBT(InputStream &sData, NBT_Node &nRoot, NBT_Node::NBT_TAG tag, size_t szStackDepth)//ѡ���������ݹ�㣬�ɺ������õĺ������
	{
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
				iRet = GetbuiltInType<NBT_Node::NBT_Byte, bHasName>(sData, nRoot);
			}
			break;
		case NBT_Node::TAG_Short:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Short, bHasName>(sData, nRoot);
			}
			break;
		case NBT_Node::TAG_Int:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Int, bHasName>(sData, nRoot);
			}
			break;
		case NBT_Node::TAG_Long:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Long, bHasName>(sData, nRoot);
			}
			break;
		case NBT_Node::TAG_Float:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Float, bHasName>(sData, nRoot);
			}
			break;
		case NBT_Node::TAG_Double:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Double, bHasName>(sData, nRoot);
			}
			break;
		case NBT_Node::TAG_Byte_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Byte_Array, bHasName>(sData, nRoot);
			}
			break;
		case NBT_Node::TAG_String:
			{
				iRet = GetStringType<bHasName>(sData, nRoot);
			}
			break;
		case NBT_Node::TAG_List://��Ҫ�ݹ���ã��б�ͷ������ǩID�ͳ��ȣ�������Ϊһϵ��ͬ���ͱ�ǩ����Ч���أ��ޱ�ǩ ID �����ƣ�
			{//���
				iRet = GetListType<bHasName>(sData, nRoot, szStackDepth);//ѡ���������ٵݹ��
			}
			break;
		case NBT_Node::TAG_Compound://��Ҫ�ݹ����
			{
				iRet = GetCompoundType<bHasName>(sData, nRoot, szStackDepth);//ѡ���������ٵݹ��
			}
			break;
		case NBT_Node::TAG_Int_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Int_Array, bHasName>(sData, nRoot);
			}
			break;
		case NBT_Node::TAG_Long_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Long_Array, bHasName>(sData, nRoot);
			}
			break;
		default://NBT�ڱ�����ǩ����
			{
				iRet = Error(NbtTypeTagError, sData, __FUNCSIG__ ": NBT Tag switch default: Unknow Type Tag[%02X(%d)]", tag, tag);//�˴���������ǰ���أ�����Ĭ�Ϸ��ش���
			}
			break;
		}

		return iRet;//���ݷ���ֵ
	}

	static int GetNBT(InputStream &sData, NBT_Node &nRoot, size_t szStackDepth)//�ݹ���ö�ȡ����ӽڵ�
	{
		CHECK_STACK_DEPTH(szStackDepth);
		//�ڵ����ͼ�飺��֤��ǰnRoot��NBT_Node::NBT_Compound���ͣ�����ʧ��
		if (!nRoot.TypeHolds<NBT_Node::NBT_Compound>())//���ʹ���
		{
			return Error(InternalTypeError, sData, __FUNCSIG__ ": nRoot is not type: [%s]", typeid(NBT_Node::NBT_Compound).name());
		}

		int iRet;
		do
		{
			if (!sData.HasAvailData(sizeof(char)))//����ĩβ���
			{
				sData.UnGet();//���Ի���һ���鿴��һ�ֽ�
				if (sData.HasAvailData(sizeof(char)) && sData.GetChar() == NBT_Node::NBT_TAG::TAG_End)//�����һ�ֽ���TAG_End����NBT�ѽ��������˳�ѭ��
				{
					iRet = Compound_End;
				}
				else
				{
					iRet = Error(OutOfRangeError, sData, __FUNCSIG__ ": Index[%zu] >= DataSize()[%zu]", sData.Index(), sData.Size());
				}
				break;
			}

			//�����������
			iRet = SwitchNBT(sData, nRoot, (NBT_Node::NBT_TAG)(uint8_t)sData.GetChar(), szStackDepth - 1);
		} while (iRet == AllOk);
		
		return iRet;//���ݷ���ֵ
	}
public:
	NBT_Reader(void) = default;
	~NBT_Reader(void) = default;

	//szStackDepth ����ջ��ȣ��ݹ������ɿ�Ƕ�׵Ŀ��ܽ��еݹ�ĺ������У�ջ��ȵݼ����ɶ�ѡ�����ĵ��ý���
	bool SetNBT(const std::string data, size_t szDataStartIndex = 0, size_t szStackDepth = 512)//����nbt������
	{//�����û���˵�����ø���
		nRoot.Clear();//���ԭ�������ݣ�ע�����nbt�ϴ������£�����һ������ĵݹ�������̣����ų�ջ�ռ䲻�㵼������ʧ�ܣ�
		size_t szCurrent{ 0 };
		InputStream sData{ data,szDataStartIndex };
		printf("Max Stack Depth [%zu]\n", szStackDepth);
		return GetNBT(sData, nRoot, szStackDepth) == Compound_End;//��������˵�Ǵ��û�����data��ã�get��nbt����
	}

	NBT_Node& GetRoot(void)
	{
		return nRoot;
	}

	const NBT_Node &GetRoot(void) const
	{
		return nRoot;
	}
private:
	NBT_Node nRoot{ NBT_Node::NBT_Compound{} };//Ĭ�Ͽսڵ�
};