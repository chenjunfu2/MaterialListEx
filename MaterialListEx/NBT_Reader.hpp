#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"

template <typename T>
class MyInputStream
{
private:
	const T &tData;
	size_t szIndex;
public:
	MyInputStream(const T &_tData, size_t szStartIdx = 0) :tData(_tData), szIndex(szStartIdx)
	{}
	~MyInputStream() = default;//Ĭ����������tData����������

	typename T::value_type GetNext()
	{
		return tData[szIndex++];
	}

	void UnGet()
	{
		if (szIndex != 0)
		{
			--szIndex;
		}
	}

	const typename T::value_type &operator[](size_t szIndex) const
	{
		return tData[szIndex];
	}

	T::const_iterator Current() const
	{
		return tData.begin() + szIndex;
	}

	T::const_iterator Next(size_t szSize) const
	{
		return tData.begin() + (szIndex + szSize);
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
		return szIndex >= tData.size();
	}

	size_t Size() const
	{
		return tData.size();
	}

	bool HasAvailData(size_t szSize) const
	{
		return (tData.size() - szIndex) >= szSize;
	}

	void Reset()
	{
		szIndex = 0;
	}

	const T &Data() const
	{
		return tData;
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

template <typename DataType = std::string>
class NBT_Reader
{
	using InputStream = MyInputStream<DataType>;//������
private:
	// ��ȡ�������ֵ��bNoCheckΪtrue�򲻽����κμ��
	template<bool bNoCheck = false, typename T>
	static std::conditional_t<bNoCheck, void, bool> ReadBigEndian(InputStream &tData, T& tVal)
	{
		if constexpr (!bNoCheck)
		{
			if (!tData.HasAvailData(sizeof(T)))
			{
				return false;
			}
		}

		if constexpr (sizeof(T) == 1)
		{
			tVal = (T)(uint8_t)tData.GetNext();
		}
		else
		{
			T tTmp = 0;
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				tTmp = (tTmp << 8) | (T)(uint8_t)tData.GetNext();//��Ϊֻ�����ƣ��������з��ŵ��µ�����λ��bug������ת��Ϊ�޷�������
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
		ERRCODE_END = -6,//������ǣ�ͳ�Ƹ������ִ�С
		ListElementTypeError = -5,//�б�Ԫ�����ʹ���NBT�ļ����⣩
		StackDepthExceeded = -4,//����ջ��ȹ��NBT�ļ�or�����������⣩
		NbtTypeTagError = -3,//NBT��ǩ���ʹ���NBT�ļ����⣩
		OutOfRangeError = -2,//��NBT�ڲ����ȴ����������NBT�ļ����⣩
		InternalTypeError = -1,//����NBT�ڵ����ʹ��󣨴������⣩
		AllOk = 0,//û������
		Compound_End = 1,//���Ͻ���
	};

	static inline const char *const errReason[] =
	{
		"AllOk",
		"InternalTypeError",
		"OutOfRangeError",
		"NbtTypeTagError",
		"StackDepthExceeded",
		"ListElementTypeError"
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
	static int _cdecl Error(T code, const InputStream &tData, _Printf_format_string_ const char *const cpExtraInfo = NULL, ...)//gccʹ��__attribute__((format))
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
			if (code <= NoWarn)
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

		size_t rangeBeg = (tData.Index() > VIEW_PRE) ? (tData.Index() - VIEW_PRE) : 0;//�ϱ߽����
		size_t rangeEnd = ((tData.Index() + VIEW_SUF) < tData.Size()) ? (tData.Index() + VIEW_SUF) : tData.Size();//�±߽����
		printf("Data Review:\nCurrent: 0x%02llX(%zu)\nData Size: 0x%02llX(%zu)\nData[0x%02llX(%zu)] ~ Data[0x%02llX(%zu)]:\n",
			(uint64_t)tData.Index(), tData.Index(), (uint64_t)tData.Size(), tData.Size(), (uint64_t)rangeBeg, rangeBeg, (uint64_t)rangeEnd - 1, rangeEnd - 1);
		
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

			if (i != tData.Index())
			{
				printf(" %02X ", (uint8_t)tData[i]);
			}
			else//����ǵ�ǰ�����ֽڣ��ӷ����ſ���
			{
				printf("[%02X]", (uint8_t)tData[i]);
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
		return Error(StackDepthExceeded, tData, _RP___FUNCSIG__ ": NBT nesting depth exceeded maximum call stack limit");\
	}\
}

	static int GetName(InputStream &tData, NBT_Node::NBT_String &sName)
	{
		//��ȡ2�ֽڵ��޷������Ƴ���
		uint16_t wNameLength = 0;//w->word=2*byte
		if (!ReadBigEndian(tData, wNameLength))
		{
			return Error(OutOfRangeError, tData, __FUNCSIG__ ": wNameLength Read");
		}

		//�жϳ����Ƿ񳬹�
		if (!tData.HasAvailData(wNameLength))
		{
			return Error(OutOfRangeError, tData, __FUNCSIG__ ": (Index[%zu] + wNameLength[%zu])[%zu] > DataSize[%zu]",
				tData.Index(), (size_t)wNameLength, tData.Index() + (size_t)wNameLength, tData.Size());
		}

		//����������
		sName = { tData.Current(), tData.Next(wNameLength) };//�������Ϊ0����0���ַ������Ϸ���Ϊ
		tData.AddIndex(wNameLength);//�ƶ��±�
		return AllOk;
	}


	template<typename T, bool bHasName = true>
	static int GetbuiltInType(InputStream &tData, NBT_Node &nRoot)
	{
		//��ȡNBT��N�����ƣ�
		NBT_Node::NBT_String sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			int iRet = GetName(tData, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//��ȡ����
		if constexpr (std::is_same<T, NBT_Node::NBT_Float>::value)//����������
		{
			uint32_t tTmpData = 0;
			if (!ReadBigEndian(tData, tTmpData))
			{
				return Error(OutOfRangeError, tData, __FUNCSIG__ ": tTmpData Read");
			}

			if constexpr (bHasName)
			{
				//����-�ں����ݵĽڵ���뵱ǰ����ջ��ȵĸ��ڵ�
				auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), std::move((*((NBT_Node::NBT_Float *)&tTmpData))));//������������ת��
				if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
				{
					Error(ElementExistsWarn, tData, __FUNCSIG__ ": the \"%s\"[%s] tData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(NBT_Node::NBT_Float).name());
				}
			}
			else
			{
				//������Ϊ�б�Ԫ�أ�ֱ���޸�nRoot
				nRoot.emplace<NBT_Node::NBT_Float>(std::move((*((NBT_Node::NBT_Float *)&tTmpData))));
			}
			
		}
		else if constexpr (std::is_same<T, NBT_Node::NBT_Double>::value)//����������
		{
			uint64_t tTmpData = 0;
			if (!ReadBigEndian(tData, tTmpData))
			{
				return Error(OutOfRangeError, tData, __FUNCSIG__ ": tTmpData Read");
			}

			if constexpr (bHasName)
			{
				//����-�ں����ݵĽڵ���뵱ǰ����ջ��ȵĸ��ڵ�
				auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), std::move((*((NBT_Node::NBT_Double *)&tTmpData))));//������������ת��
				if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
				{
					Error(ElementExistsWarn, tData, __FUNCSIG__ ": the \"%s\"[%s] tData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(NBT_Node::NBT_Double).name());
				}
			}
			else
			{
				//������Ϊ�б�Ԫ�أ�ֱ���޸�nRoot
				nRoot.emplace<NBT_Node::NBT_Double>(std::move((*((NBT_Node::NBT_Double *)&tTmpData))));
			}
		}
		else if constexpr (std::is_integral<T>::value)
		{
			T tTmpData = 0;
			if (!ReadBigEndian(tData, tTmpData))
			{
				return Error(OutOfRangeError, tData, __FUNCSIG__ ": tTmpData Read");
			}

			if constexpr (bHasName)
			{
				//����-�ں����ݵĽڵ���뵱ǰ����ջ��ȵĸ��ڵ�
				auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), std::move(tTmpData));
				if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
				{
					Error(ElementExistsWarn, tData, __FUNCSIG__ ": the \"%s\"[%s] tData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(tTmpData).name());
				}
			}
			else
			{
				nRoot.emplace<T>(std::move(tTmpData));
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
	static int GetArrayType(InputStream &tData, NBT_Node &nRoot)
	{
		//�ж��ǲ���vector<x>
		if constexpr (!is_std_vector<T>::value)
		{
			static_assert(false, "Not a legal type call!");//�׳��������
		}

		//��ȡNBT��N�����ƣ�
		NBT_Node::NBT_String sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			int iRet = GetName(tData, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//��ȡ4�ֽ��з���������������Ԫ�ظ���
		int32_t dwElementCount = 0;//dw->double-word=4*byte
		if (!ReadBigEndian(tData, dwElementCount))
		{
			return Error(OutOfRangeError, tData, __FUNCSIG__ ": dwElementCount Read");
		}

		//�жϳ����Ƿ񳬹�
		if (!tData.HasAvailData(dwElementCount * sizeof(T::value_type)))//��֤�·����ð�ȫ
		{
			return Error(OutOfRangeError, tData, __FUNCSIG__ ": (Index[%zu] + dwElementCount[%zu] * sizeof(T::value_type)[%zu])[%zu] > DataSize[%zu]", 
				tData.Index(), (size_t)dwElementCount, sizeof(T::value_type), tData.Index() + (size_t)dwElementCount * sizeof(T::value_type), tData.Size());
		}
		
		//���鱣��
		T tArray;
		tArray.reserve(dwElementCount);//��ǰ����

		//��ȡdElementCount��Ԫ��
		for (int32_t i = 0; i < dwElementCount; ++i)
		{
			typename T::value_type tTmpData;
			ReadBigEndian<true>(tData, tTmpData);//������Ҫȷ����Χ��ȫ
			tArray.emplace_back(std::move(tTmpData));//��ȡһ������һ��
		}
		
		if constexpr (bHasName)
		{
			//��ɺ����
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), std::move(tArray));
			if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
			{
				Error(ElementExistsWarn, tData, __FUNCSIG__ ": the \"%s\"[%s] tData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(tArray).name());
			}
		}
		else//�����ƣ�Ϊ�б�Ԫ��
		{
			nRoot.emplace<T>(std::move(tArray));
		}
		return AllOk;
	}

	template<bool bHasName = true>
	static int GetCompoundType(InputStream &tData, NBT_Node &nRoot, size_t szStackDepth)//�˵ݹ麯����Ҫ��֤���ݷ���ֵ
	{
		CHECK_STACK_DEPTH(szStackDepth);
		//��ȡNBT��N�����ƣ�
		NBT_Node::NBT_String sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			int iRet = GetName(tData, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//��ʼ�ݹ�
		NBT_Node nodeTemp{ NBT_Node::NBT_Compound{} };
		int iRet = GetNBT(tData, nodeTemp, szStackDepth);//�˴�ֱ�Ӵ��ݣ���ΪGetNBT�ڲ�����еݼ�
		if (iRet < AllOk)
		{
			return iRet;
		}

		if constexpr (bHasName)
		{
			//�ݹ���ɣ������ӽڵ��ѵ�λ
			//ȡ��NBT_Compound�ҵ��Լ��������ƶ���
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), std::move(nodeTemp));
			if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
			{
				Error(ElementExistsWarn, tData, __FUNCSIG__ ": the \"%s\"[%s] tData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(NBT_Node::NBT_Compound).name());
			}
		}
		else//�����ƣ�Ϊ�б�Ԫ��
		{
			nRoot = std::move(nodeTemp);
		}

		return AllOk;//�����ӽڵ㷵��ok
	}

	template<bool bHasName = true>
	static int GetStringType(InputStream &tData, NBT_Node &nRoot)
	{
		//��ȡNBT��N�����ƣ�
		NBT_Node::NBT_String sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			int iRet = GetName(tData, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//��ȡ2�ֽڵ��޷������Ƴ���
		uint16_t wStrLength = 0;//w->word=2*byte
		if (!ReadBigEndian(tData, wStrLength))
		{
			return Error(OutOfRangeError, tData, __FUNCSIG__ ": wStrLength Read");
		}

		//�жϳ����Ƿ񳬹�
		if (!tData.HasAvailData(wStrLength))
		{
			return Error(OutOfRangeError, tData, __FUNCSIG__ ": (Index[%zu] + wStrLength[%zu])[%zu] > DataSize[%zu]",
				tData.Index(), (size_t)wStrLength, tData.Index() + (size_t)wStrLength, tData.Size());
		}

		if constexpr (bHasName)
		{
			//ԭλ����
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), std::in_place_type<NBT_Node::NBT_String>, tData.Current(), tData.Next(wStrLength));//�ӵ��������з�Χ���죨����move��������
			if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
			{
				Error(ElementExistsWarn, tData, __FUNCSIG__ ": the \"%s\"[%s] tData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(NBT_Node::NBT_String).name());
			}
		}
		else//�б�Ԫ��ֱ�Ӹ�ֵ
		{
			nRoot.emplace<NBT_Node::NBT_String>(tData.Current(), tData.Next(wStrLength));//�ӵ��������з�Χ���죨����move��������
		}
		tData.AddIndex(wStrLength);//�ƶ��±�
		
		return AllOk;
	}

	template<bool bHasName = true>
	static int GetListType(InputStream &tData, NBT_Node &nRoot, size_t szStackDepth)
	{
		CHECK_STACK_DEPTH(szStackDepth);
		//��ȡNBT��N�����ƣ�
		NBT_Node::NBT_String sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			int iRet = GetName(tData, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//��ȡ1�ֽڵ��б�Ԫ������
		uint8_t bListElementType = 0;//b=byte
		if (!ReadBigEndian(tData, bListElementType))
		{
			return Error(OutOfRangeError, tData, __FUNCSIG__ ": bListElementType Read");
		}

		//������б�Ԫ������
		if (bListElementType >= NBT_Node::NBT_TAG::ENUM_END)
		{
			return Error(NbtTypeTagError, tData, __FUNCSIG__ ": List NBT Type:Unknow Type Tag[%02X(%d)]", bListElementType, bListElementType);
		}

		//��ȡ4�ֽڵ��з����б���
		int32_t dwListLength = 0;//dw=double-world=4*byte
		if (!ReadBigEndian(tData, dwListLength))
		{
			return Error(OutOfRangeError, tData, __FUNCSIG__ ": dwListLength Read");
		}

		//����Ԫ�����ͣ���ȡn���б�
		NBT_Node::NBT_List tmpList;
		tmpList.reserve(dwListLength);//��֪��С��ǰ������ٿ���

		for (int32_t i = 0; i < dwListLength; ++i)
		{
			NBT_Node tmpNode{};//�б�Ԫ�ػ�ֱ�Ӹ�ֵ�޸�
			int iRet = SwitchNBT<false>(tData, tmpNode, (NBT_Node::NBT_TAG)bListElementType, szStackDepth - 1);

			if (iRet < AllOk)//������
			{
				return iRet;//�˴�ֻ��ʧ��ʱ���ݷ���ֵ
			}

			if (bListElementType == NBT_Node::NBT_TAG::TAG_End)//��Ԫ������
			{
				//��ȡ1�ֽڵ�bCompoundEndTag
				uint8_t bCompoundEndTag = 0;//b=byte
				if (!ReadBigEndian(tData, bCompoundEndTag))
				{
					return Error(OutOfRangeError, tData, __FUNCSIG__ ": bCompoundEndTag Read");
				}

				//�ж�tag�Ƿ����
				if (bCompoundEndTag != NBT_Node::TAG_End)
				{
					return Error(ListElementTypeError, tData, __FUNCSIG__ ": require Compound End Tag [0x00], but read Unknow Tag [0x%02X]!", bCompoundEndTag);
				}
			}

			//ÿ��ȡһ���������һ��
			tmpList.emplace_back(std::move(tmpNode));
		}

		//�б��Ƕ�ף����Դ�����Ƕ���������
		if constexpr (bHasName)
		{
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), std::move(tmpList));
			if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
			{
				//�˴�ʵΪ������Ǵ�������return
				Error(ElementExistsWarn, tData, __FUNCSIG__ ": the \"%s\"[%s] tData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(tmpList).name());
			}
		}
		else//�б��е��б�ֱ�Ӹ�ֵ���������в���
		{
			nRoot.emplace<NBT_Node::NBT_List>(std::move(tmpList));
		}

		return AllOk;//�б�ͬʱ��ΪԪ�أ��ɹ�Ӧ�÷���Ok�������Ǵ��ݷ���ֵ
	}

	template<bool bHasName = true>
	static int SwitchNBT(InputStream &tData, NBT_Node &nRoot, NBT_Node::NBT_TAG tag, size_t szStackDepth)//ѡ���������ݹ�㣬�ɺ������õĺ������
	{
		int iRet = AllOk;

		switch (tag)
		{
		case NBT_Node::TAG_End:
			{
				iRet = Compound_End;//���ؽ�β
			}
			break;
		case NBT_Node::TAG_Byte:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Byte, bHasName>(tData, nRoot);
			}
			break;
		case NBT_Node::TAG_Short:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Short, bHasName>(tData, nRoot);
			}
			break;
		case NBT_Node::TAG_Int:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Int, bHasName>(tData, nRoot);
			}
			break;
		case NBT_Node::TAG_Long:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Long, bHasName>(tData, nRoot);
			}
			break;
		case NBT_Node::TAG_Float:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Float, bHasName>(tData, nRoot);
			}
			break;
		case NBT_Node::TAG_Double:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Double, bHasName>(tData, nRoot);
			}
			break;
		case NBT_Node::TAG_Byte_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Byte_Array, bHasName>(tData, nRoot);
			}
			break;
		case NBT_Node::TAG_String:
			{
				iRet = GetStringType<bHasName>(tData, nRoot);
			}
			break;
		case NBT_Node::TAG_List://��Ҫ�ݹ���ã��б�ͷ������ǩID�ͳ��ȣ�������Ϊһϵ��ͬ���ͱ�ǩ����Ч���أ��ޱ�ǩ ID �����ƣ�
			{//���
				iRet = GetListType<bHasName>(tData, nRoot, szStackDepth);//ѡ���������ٵݹ��
			}
			break;
		case NBT_Node::TAG_Compound://��Ҫ�ݹ����
			{
				iRet = GetCompoundType<bHasName>(tData, nRoot, szStackDepth);//ѡ���������ٵݹ��
			}
			break;
		case NBT_Node::TAG_Int_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Int_Array, bHasName>(tData, nRoot);
			}
			break;
		case NBT_Node::TAG_Long_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Long_Array, bHasName>(tData, nRoot);
			}
			break;
		default://NBT�ڱ�����ǩ����
			{
				iRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": NBT Tag switch default: Unknow Type Tag[%02X(%d)]", tag, tag);//�˴���������ǰ���أ�����Ĭ�Ϸ��ش���
			}
			break;
		}

		return iRet;//���ݷ���ֵ
	}

	template<bool bRoot = false>//�����ػ��汾�����ڴ���ĩβ
	static int GetNBT(InputStream &tData, NBT_Node &nRoot, size_t szStackDepth)//�ݹ���ö�ȡ����ӽڵ�
	{
		CHECK_STACK_DEPTH(szStackDepth);
		//�ڵ����ͼ�飺��֤��ǰnRoot��NBT_Node::NBT_Compound���ͣ�����ʧ��
		if (!nRoot.TypeHolds<NBT_Node::NBT_Compound>())//���ʹ���
		{
			return Error(InternalTypeError, tData, __FUNCSIG__ ": nRoot is not type: [%s]", typeid(NBT_Node::NBT_Compound).name());
		}

		int iRet = AllOk;//Ĭ��ֵ
		do
		{
			if (!tData.HasAvailData(sizeof(uint8_t)))//����ĩβ���
			{
				if constexpr (!bRoot)//�Ǹ����������ĩβ���򱨴�
				{
					iRet = Error(OutOfRangeError, tData, __FUNCSIG__ ": Index[%zu] >= DataSize()[%zu]", tData.Index(), tData.Size());
					break;
				}
				else
				{
					iRet = AllOk;
					break;//�������ֱ������
				}
			}

			//�����������
			iRet = SwitchNBT(tData, nRoot, (NBT_Node::NBT_TAG)(uint8_t)tData.GetNext(), szStackDepth - 1);
		} while (iRet == AllOk);//iRet<AllOk��Ϊ��������ѭ����>AllOk��Ϊ��������������ѭ��

		if constexpr (bRoot)//�����������Compound_End��תΪAllOk����
		{
			if (iRet == Compound_End)
			{
				iRet = AllOk;
			}
		}
		
		return iRet;//���ݷ���ֵ
	}
public:
	NBT_Reader(void) = delete;
	~NBT_Reader(void) = delete;

	//szStackDepth ����ջ��ȣ��ݹ������ɿ�Ƕ�׵Ŀ��ܽ��еݹ�ĺ������У�ջ��ȵݼ����ɶ�ѡ�����ĵ��ý���
	static bool ReadNBT(NBT_Node &nRoot, const DataType &tData, size_t szDataStartIndex = 0, size_t szStackDepth = 512)//��data�ж�ȡnbt
	{
		nRoot.Clear();//���ԭ�������ݣ�ע�����nbt�ϴ������£�����һ������ĵݹ�������̣����ų�ջ�ռ䲻�㵼������ʧ�ܣ�
		printf("Max Stack Depth [%zu]\n", szStackDepth);
		InputStream IptStream{ tData,szDataStartIndex };
		return GetNBT<true>(IptStream, nRoot, szStackDepth) == AllOk;//��data�л�ȡnbt���ݵ�nRoot�У�ֻ�д˵���Ϊ�������ã�ģ��true�������ڴ����������
	}
};