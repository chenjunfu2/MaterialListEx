#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"

#include <new>//std::bad_alloc

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

	typename T::value_type GetNext() noexcept
	{
		return tData[szIndex++];
	}

	void UnGet() noexcept
	{
		if (szIndex != 0)
		{
			--szIndex;
		}
	}

	const typename T::value_type &operator[](size_t szIndex) const noexcept
	{
		return tData[szIndex];
	}

	T::const_iterator Current() const noexcept
	{
		return tData.begin() + szIndex;
	}

	T::const_iterator Next(size_t szSize) const noexcept
	{
		return tData.begin() + (szIndex + szSize);
	}

	size_t AddIndex(size_t szSize) noexcept
	{
		return szIndex += szSize;
	}

	size_t SubIndex(size_t szSize) noexcept
	{
		return szIndex -= szSize;
	}

	bool IsEnd() const noexcept
	{
		return szIndex >= tData.size();
	}

	size_t Size() const noexcept
	{
		return tData.size();
	}

	bool HasAvailData(size_t szSize) const noexcept
	{
		return (tData.size() - szIndex) >= szSize;
	}

	void Reset() noexcept
	{
		szIndex = 0;
	}

	const T &Data() const noexcept
	{
		return tData;
	}

	size_t Index() const noexcept
	{
		return szIndex;
	}

	size_t &Index() noexcept
	{
		return szIndex;
	}
};

template <typename DataType = std::string>
class NBT_Reader
{
	using InputStream = MyInputStream<DataType>;//������
private:
	enum ErrCode : int
	{
		ERRCODE_END = -9,//������ǣ�ͳ�Ƹ������ִ�С

		UnknownError,//��������
		StdException,//��׼�쳣
		OutOfMemoryError,//�ڴ治�����NBT�ļ����⣩
		ListElementTypeError,//�б�Ԫ�����ʹ���NBT�ļ����⣩
		StackDepthExceeded,//����ջ��ȹ��NBT�ļ�or�����������⣩
		NbtTypeTagError,//NBT��ǩ���ʹ���NBT�ļ����⣩
		OutOfRangeError,//��NBT�ڲ����ȴ����������NBT�ļ����⣩
		InternalTypeError,//����NBT�ڵ����ʹ��󣨴������⣩
		AllOk,//û������
		Compound_End,//���Ͻ���
	};

	//ȷ��[�Ǵ�����]Ϊ�㣬��ֹ���ַǷ���[�Ǵ�����]�����ж�ʧЧ�������
	static_assert(AllOk == 0, "AllOk != 0");

	static inline const char *const errReason[] =//�����������㷽ʽ��(-ERRCODE_END - 1) + ErrCode
	{
		"UnknownError",
		"StdException",
		"OutOfMemoryError",
		"ListElementTypeError",
		"StackDepthExceeded",
		"NbtTypeTagError",
		"OutOfRangeError",
		"InternalTypeError",

		"AllOk",
	};

	//�ǵ�ͬ�����飡
	static_assert(sizeof(errReason) / sizeof(errReason[0]) == (-ERRCODE_END), "errReason array out sync");

	enum WarnCode : int
	{
		NoWarn = 0,
		ElementExistsWarn,

		WARNCODE_END,
	};

	//ȷ��[�Ǿ�����]Ϊ�㣬��ֹ���ַǷ���[�Ǿ�����]�����ж�ʧЧ�������
	static_assert(NoWarn == 0, "NoWarn != 0");

	static inline const char *const warnReason[] =//�������飬ֱ����WarnCode����
	{
		"NoWarn",
		"ElementExistsWarn",
	};

	//�ǵ�ͬ�����飡
	static_assert(sizeof(warnReason) / sizeof(warnReason[0]) == WARNCODE_END, "warnReason array out sync");

	//ʹ�ñ���βα�+vprintf���������������������չ��Ϣ
	template <typename T, typename std::enable_if<std::is_same<T, ErrCode>::value || std::is_same<T, WarnCode>::value, int>::type = 0>
	static int _cdecl Error(const T &code, const InputStream &tData, _Printf_format_string_ const char *const cpExtraInfo = NULL, ...) noexcept//gccʹ��__attribute__((format))
	{
		if constexpr (std::is_same<T, ErrCode>::value)
		{
			if (code >= AllOk)
			{
				return (int)code;
			}
			//�Ϸ�if��֤errcΪ�����˴����ʱ�֤�����⣨���Ǵ��봫���쳣�����룩��ͨ��(-ERRCODE_END - 1) + code�õ����������±�
			printf("Read Err[%d]: \"%s\"\n", (int)code, errReason[(-ERRCODE_END - 1) + code]);
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
			static_assert(false, "Unknown [T code] Type!");
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
#undef VIEW_SUF
#undef VIEW_PRE
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
			printf("\nSkip err data and return...\n\n");
		}
		else if constexpr (std::is_same<T, WarnCode>::value)
		{
			printf("\nSkip warn data and continue...\n\n");
		}
		else
		{
			static_assert(false, "Unknown [T code] Type!");
		}

		return (int)code;
	}

#define _RP___FUNCSIG__ __FUNCSIG__//���ڱ�����̶����滻�ﵽ�����ڲ�

#define _RP___LINE__ _RP_STRLING(__LINE__)
#define _RP_STRLING(l) STRLING(l)
#define STRLING(l) #l

#define STACK_TRACEBACK(fmt, ...) printf("In [" _RP___FUNCSIG__ "] Line:[" _RP___LINE__ "]: \n"##fmt "\n\n", ##__VA_ARGS__);
#define CHECK_STACK_DEPTH(Depth) \
{\
	if((Depth) <= 0)\
	{\
		iRet = Error(StackDepthExceeded, tData, _RP___FUNCSIG__ ": NBT nesting depth exceeded maximum call stack limit");\
		STACK_TRACEBACK("(Depth) <= 0");\
		return iRet;\
	}\
}

#define MYTRY \
try\
{

#define MYCATCH_BADALLOC \
}\
catch(const std::bad_alloc &e)\
{\
	int iRet = Error(OutOfMemoryError, tData, _RP___FUNCSIG__ ": Info:[%s]", e.what());\
	STACK_TRACEBACK("catch(std::bad_alloc)");\
	return iRet;\
}

#define MYCATCH_OTHER \
}\
catch(const std::exception &e)\
{\
	int iRet = Error(StdException, tData, _RP___FUNCSIG__ ": Info:[%s]", e.what());\
	STACK_TRACEBACK("catch(std::exception)");\
	return iRet;\
}\
catch(...)\
{\
	int iRet =  Error(UnknownError, tData, _RP___FUNCSIG__ ": Info:[Unknown Exception]");\
	STACK_TRACEBACK("catch(...)");\
	return iRet;\
}


	// ��ȡ�������ֵ��bNoCheckΪtrue�򲻽����κμ��
	template<bool bNoCheck = false, typename T>
	static std::conditional_t<bNoCheck, void, int> ReadBigEndian(InputStream &tData, T &tVal) noexcept
	{
		if constexpr (!bNoCheck)
		{
			if (!tData.HasAvailData(sizeof(T)))
			{
				int iRet = Error(OutOfRangeError, tData, "tData size [%zu], current index [%zu], remaining data size [%zu], but try to read [%zu]",
					tData.Size(), tData.Index(), tData.Size() - tData.Index(), sizeof(T));
				STACK_TRACEBACK("HasAvailData");
				return iRet;
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
				tTmp <<= 8;
				tTmp |= (T)(uint8_t)tData.GetNext();//��Ϊֻ�����ƣ��������з��ŵ��µ�����λ��bug������ת��Ϊ�޷�������
			}
			tVal = tTmp;
		}

		if constexpr (!bNoCheck)
		{
			return AllOk;
		}
	}

	static int GetName(InputStream &tData, NBT_Node::NBT_String &sName)
	{
		int iRet = AllOk;
		//��ȡ2�ֽڵ��޷������Ƴ���
		uint16_t wNameLength = 0;//w->word=2*byte
		iRet = ReadBigEndian(tData, wNameLength);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("wNameLength Read");
			return iRet;
		}

		//�жϳ����Ƿ񳬹�
		if (!tData.HasAvailData(wNameLength))
		{
			int iRet = Error(OutOfRangeError, tData, __FUNCSIG__ ": (Index[%zu] + wNameLength[%zu])[%zu] > DataSize[%zu]",
				tData.Index(), (size_t)wNameLength, tData.Index() + (size_t)wNameLength, tData.Size());
			STACK_TRACEBACK("HasAvailData");
			return iRet;
		}

		MYTRY
		//����������
		sName.reserve(wNameLength);//��ǰ����
		sName.assign(tData.Current(), tData.Next(wNameLength));//�������Ϊ0����0���ַ������Ϸ���Ϊ
		MYCATCH_BADALLOC
		tData.AddIndex(wNameLength);//�ƶ��±�
		return AllOk;
	}

	template<typename T>
	struct BUILTIN_TYPE
	{
		using Type = T;
		static_assert(std::is_integral<T>::value, "Not a legal type!");//�׳��������
	};

	template<>
	struct BUILTIN_TYPE<NBT_Node::NBT_Float>//������ӳ��
	{
		using Type = uint32_t;
		static_assert(sizeof(Type) == sizeof(NBT_Node::NBT_Float), "Type size does not match!");
	};

	template<>
	struct BUILTIN_TYPE<NBT_Node::NBT_Double>//������ӳ��
	{
		using Type = uint64_t;
		static_assert(sizeof(Type) == sizeof(NBT_Node::NBT_Double), "Type size does not match!");
	};

	template<typename T, bool bHasName = true>
	static int GetbuiltInType(InputStream &tData, NBT_Node &nRoot)
	{
		int iRet = AllOk;
		//��ȡNBT��N�����ƣ�
		NBT_Node::NBT_String sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			iRet = GetName(tData, sName);
			if (iRet < AllOk)
			{
				STACK_TRACEBACK("GetName");
				return iRet;
			}
		}

		//��ȡ����
		using DATA_T = BUILTIN_TYPE<T>::Type;//����ӳ��

		DATA_T tTmpData = 0;
		iRet = ReadBigEndian(tData, tTmpData);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("Name: \"%s\" tTmpData Read", sName.c_str());
			return iRet;
		}

		if constexpr (bHasName)
		{
			MYTRY
			//����-�ں����ݵĽڵ���뵱ǰ����ջ��ȵĸ��ڵ�
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), std::move((*((T *)&tTmpData))));//������������ת��
			if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
			{
				Error(ElementExistsWarn, tData, __FUNCSIG__ ": the \"%s\"[%s] tData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(T).name());
			}
			MYCATCH_BADALLOC
		}
		else
		{
			//������Ϊ�б�Ԫ�أ�ֱ���޸�nRoot
			nRoot.emplace<T>(std::move((*((T *)&tTmpData))));
		}

		return iRet;
	}

	template<typename T, bool bHasName = true>
	static int GetArrayType(InputStream &tData, NBT_Node &nRoot)
	{
		int iRet = AllOk;
		//��ȡNBT��N�����ƣ�
		NBT_Node::NBT_String sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			iRet = GetName(tData, sName);
			if (iRet < AllOk)
			{
				STACK_TRACEBACK("GetName");
				return iRet;
			}
		}

		//��ȡ4�ֽ��з���������������Ԫ�ظ���
		int32_t dwElementCount = 0;//dw->double-word=4*byte
		iRet = ReadBigEndian(tData, dwElementCount);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("Name: \"%s\" dwElementCount Read", sName.c_str());
			return iRet;
		}

		//�жϳ����Ƿ񳬹�
		if (!tData.HasAvailData(dwElementCount * sizeof(T::value_type)))//��֤�·����ð�ȫ
		{
			iRet = Error(OutOfRangeError, tData, __FUNCSIG__ ": (Index[%zu] + dwElementCount[%zu] * sizeof(T::value_type)[%zu])[%zu] > DataSize[%zu]", 
				tData.Index(), (size_t)dwElementCount, sizeof(T::value_type), tData.Index() + (size_t)dwElementCount * sizeof(T::value_type), tData.Size());
			STACK_TRACEBACK("Name: \"%s\"", sName.c_str());
			return iRet;
		}
		
		//���鱣��
		T tArray{};
		MYTRY
		tArray.reserve(dwElementCount);//��ǰ����
		//��ȡdElementCount��Ԫ��
		for (int32_t i = 0; i < dwElementCount; ++i)
		{
			typename T::value_type tTmpData;
			ReadBigEndian<true>(tData, tTmpData);//������Ҫȷ����Χ��ȫ
			tArray.emplace_back(std::move(tTmpData));//��ȡһ������һ��
		}
		MYCATCH_BADALLOC
		
		if constexpr (bHasName)
		{
			MYTRY
			//��ɺ����
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), std::move(tArray));
			if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
			{
				Error(ElementExistsWarn, tData, __FUNCSIG__ ": the \"%s\"[%s] tData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(tArray).name());
			}
			MYCATCH_BADALLOC
		}
		else//�����ƣ�Ϊ�б�Ԫ��
		{
			nRoot.emplace<T>(std::move(tArray));
		}

		return iRet;
	}

	template<bool bHasName = true>
	static int GetCompoundType(InputStream &tData, NBT_Node &nRoot, size_t szStackDepth)//�˵ݹ麯����Ҫ��֤���ݷ���ֵ
	{
		int iRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);
		//��ȡNBT��N�����ƣ�
		NBT_Node::NBT_String sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			iRet = GetName(tData, sName);
			if (iRet < AllOk)
			{
				STACK_TRACEBACK("GetName");
				return iRet;
			}
		}

		//��ʼ�ݹ�
		NBT_Node nodeTemp{ NBT_Node::NBT_Compound{} };
		//�˴����貶���쳣����ΪGetNBT�ڲ����õ�switchNBT�����׳��쳣
		iRet = GetNBT(tData, nodeTemp, szStackDepth);//�˴�ֱ�Ӵ��ݣ���ΪGetNBT�ڲ�����еݼ�
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("Name: \"%s\" GetNBT", sName.c_str());
			//return iRet;//�����������в��룬�Ա��������֮ǰ����ȷ����
		}

		if constexpr (bHasName)
		{
			MYTRY
			//�ݹ���ɣ������ӽڵ��ѵ�λ
			//ȡ��NBT_Compound�ҵ��Լ��������ƶ���
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), std::move(nodeTemp));
			if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
			{
				Error(ElementExistsWarn, tData, __FUNCSIG__ ": the \"%s\"[%s] tData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(NBT_Node::NBT_Compound).name());
			}
			MYCATCH_BADALLOC
		}
		else//�����ƣ�Ϊ�б�Ԫ��
		{
			nRoot = std::move(nodeTemp);
		}

		return iRet >= AllOk ? AllOk : iRet;//>=AllOkǿ�Ʒ���AllOk�Է�ֹCompound_End
	}

	template<bool bHasName = true>
	static int GetStringType(InputStream &tData, NBT_Node &nRoot)
	{
		int iRet = AllOk;
		//��ȡNBT��N�����ƣ�
		NBT_Node::NBT_String sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			iRet = GetName(tData, sName);
			if (iRet < AllOk)
			{
				STACK_TRACEBACK("GetName");
				return iRet;
			}
		}

		//��ȡ2�ֽڵ��޷������Ƴ���
		uint16_t wStrLength = 0;//w->word=2*byte
		iRet = ReadBigEndian(tData, wStrLength);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("Name: \"%s\" wStrLength Read", sName.c_str());
			return iRet;
		}

		//�жϳ����Ƿ񳬹�
		if (!tData.HasAvailData(wStrLength))
		{
			iRet = Error(OutOfRangeError, tData, __FUNCSIG__ ": (Index[%zu] + wStrLength[%zu])[%zu] > DataSize[%zu]",
				tData.Index(), (size_t)wStrLength, tData.Index() + (size_t)wStrLength, tData.Size());
			STACK_TRACEBACK("Name: \"%s\"", sName.c_str());
			return iRet;
		}

		//��ȡ�ַ���
		NBT_Node::NBT_String sString{};
		MYTRY
		sString.reserve(wStrLength);//Ԥ����
		sString.assign(tData.Current(), tData.Next(wStrLength));//��Χ��ֵ
		MYCATCH_BADALLOC
		tData.AddIndex(wStrLength);//�ƶ��±�

		if constexpr (bHasName)
		{
			MYTRY
			//ԭλ����
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), std::move(sString));
			if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
			{
				Error(ElementExistsWarn, tData, __FUNCSIG__ ": the \"%s\"[%s] tData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(NBT_Node::NBT_String).name());
			}
			MYCATCH_BADALLOC
		}
		else//�б�Ԫ��ֱ�Ӹ�ֵ
		{
			nRoot.emplace<NBT_Node::NBT_String>(std::move(sString));
		}
		
		return iRet;
	}

	template<bool bHasName = true>
	static int GetListType(InputStream &tData, NBT_Node &nRoot, size_t szStackDepth)
	{
		int iRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);
		//��ȡNBT��N�����ƣ�
		NBT_Node::NBT_String sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			iRet = GetName(tData, sName);
			if (iRet < AllOk)
			{
				STACK_TRACEBACK("GetName");
				return iRet;
			}
		}

		//��ȡ1�ֽڵ��б�Ԫ������
		uint8_t bListElementType = 0;//b=byte
		iRet = ReadBigEndian(tData, bListElementType);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("Name: \"%s\" bListElementType Read", sName.c_str());
			return iRet;
		}

		//������б�Ԫ������
		if (bListElementType >= NBT_Node::NBT_TAG::ENUM_END)
		{
			iRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": List NBT Type:Unknown Type Tag[%02X(%d)]", bListElementType, bListElementType);
			STACK_TRACEBACK("Name: \"%s\"", sName.c_str());
			return iRet;
		}

		//��ȡ4�ֽڵ��з����б���
		int32_t dwListLength = 0;//dw=double-world=4*byte
		iRet = ReadBigEndian(tData, dwListLength);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("Name: \"%s\" dwListLength Read", sName.c_str());
			return iRet;
		}

		//����з�������С��Χ
		if (dwListLength < 0)
		{
			iRet = Error(OutOfRangeError, tData, __FUNCSIG__ ": dwListLength[%d] < 0", dwListLength);
			STACK_TRACEBACK("Name: \"%s\"", sName.c_str());
			return iRet;
		}

		//����Ԫ�����ͣ���ȡn���б�
		NBT_Node::NBT_List tmpList{};
		MYTRY
		tmpList.reserve(dwListLength);//��֪��С��ǰ������ٿ���

		for (int32_t i = 0; i < dwListLength; ++i)
		{
			NBT_Node tmpNode{};//�б�Ԫ�ػ�ֱ�Ӹ�ֵ�޸�
			iRet = SwitchNBT<false>(tData, tmpNode, (NBT_Node::NBT_TAG)bListElementType, szStackDepth - 1);
			if (iRet < AllOk)//������
			{
				STACK_TRACEBACK("Name: \"%s\" Size: [%d] Index: [%d]", sName.c_str(), dwListLength, i);
				break;//����ѭ���Ա�����������֮ǰ����ȷ����
			}

			if (bListElementType == NBT_Node::NBT_TAG::TAG_End)//��Ԫ������
			{
				//��ȡ1�ֽڵ�bCompoundEndTag
				uint8_t bCompoundEndTag = 0;//b=byte
				iRet = ReadBigEndian(tData, bCompoundEndTag);
				if (iRet < AllOk)
				{
					STACK_TRACEBACK("Name: \"%s\" Size: [%d] Index: [%d] bCompoundEndTag Read", sName.c_str(), dwListLength, i);
					break;//����ѭ���Ա�����������֮ǰ����ȷ����
				}

				//�ж�tag�Ƿ����
				if (bCompoundEndTag != NBT_Node::TAG_End)
				{
					iRet = Error(ListElementTypeError, tData, __FUNCSIG__ ": require Compound End Tag [0x00], but read Unknown Tag [0x%02X]!", bCompoundEndTag);
					STACK_TRACEBACK("Name: \"%s\" Size: [%d] Index: [%d]", sName.c_str(), dwListLength, i);
					break;//����ѭ���Ա�����������֮ǰ����ȷ����
				}
			}

			//ÿ��ȡһ���������һ��
			tmpList.emplace_back(std::move(tmpNode));
		}
		MYCATCH_BADALLOC

		//�б��Ƕ�ף����Դ�����Ƕ���������
		if constexpr (bHasName)
		{
			MYTRY
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), std::move(tmpList));
			if (!ret.second)//����ʧ�ܣ�Ԫ���Ѵ���
			{
				//�˴�ʵΪ������Ǵ�������return
				Error(ElementExistsWarn, tData, __FUNCSIG__ ": the \"%s\"[%s] tData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(tmpList).name());
			}
			MYCATCH_BADALLOC
		}
		else//�б��е��б�ֱ�Ӹ�ֵ���������в���
		{
			nRoot.emplace<NBT_Node::NBT_List>(std::move(tmpList));
		}

		//��������򴫵ݣ����򷵻�Ok
		return iRet >= AllOk ? AllOk : iRet;//�б�ͬʱ��ΪԪ�أ��ɹ�Ӧ�÷���Ok�������Ǵ��ݷ���ֵ
	}

	//����������������ڲ����ò������쳣�������أ����Դ˺������Բ��׳��쳣���ɴ˵��ô˺����ĺ���Ҳ������catch�쳣
	template<bool bHasName = true>
	static int SwitchNBT(InputStream &tData, NBT_Node &nRoot, NBT_Node::NBT_TAG tag, size_t szStackDepth) noexcept//ѡ���������ݹ�㣬�ɺ������õĺ������
	{
		int iRet = AllOk;

		MYTRY
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
				iRet = GetArrayType<NBT_Node::NBT_ByteArray, bHasName>(tData, nRoot);
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
				iRet = GetArrayType<NBT_Node::NBT_IntArray, bHasName>(tData, nRoot);
			}
			break;
		case NBT_Node::TAG_Long_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_LongArray, bHasName>(tData, nRoot);
			}
			break;
		default://NBT�ڱ�����ǩ����
			{
				iRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": NBT Tag switch default: Unknown Type Tag[%02X(%d)]", tag, tag);//�˴���������ǰ���أ�����Ĭ�Ϸ��ش���
			}
			break;
		}
		MYCATCH_OTHER//���ﲶ����������δ������쳣

		if (iRet < AllOk)
		{
			STACK_TRACEBACK("iRet < AllOk");
		}

		return iRet;//���ݷ���ֵ
	}

	//���׳��쳣
	template<bool bRoot = false>//�����ػ��汾�����ڴ���ĩβ
	static int GetNBT(InputStream &tData, NBT_Node &nRoot, size_t szStackDepth) noexcept//�ݹ���ö�ȡ����ӽڵ�
	{
		int iRet = AllOk;//Ĭ��ֵ
		CHECK_STACK_DEPTH(szStackDepth);
		//�ڵ����ͼ�飺��֤��ǰnRoot��NBT_Node::NBT_Compound���ͣ�����ʧ��
		if (!nRoot.TypeHolds<NBT_Node::NBT_Compound>())//���ʹ���
		{
			iRet = Error(InternalTypeError, tData, __FUNCSIG__ ": nRoot is not type: [%s]", typeid(NBT_Node::NBT_Compound).name());
			STACK_TRACEBACK("TypeHolds");
			return iRet;
		}
		
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
			iRet = SwitchNBT(tData, nRoot, (NBT_Node::NBT_TAG)(uint8_t)tData.GetNext(), szStackDepth - 1);//�Ѳ��������쳣
		} while (iRet == AllOk);//iRet<AllOk��Ϊ��������ѭ����>AllOk��Ϊ��������������ѭ��

		if constexpr (bRoot)//�����������Compound_End��תΪAllOk����
		{
			if (iRet >= AllOk)
			{
				iRet = AllOk;
			}
		}

		if (iRet < AllOk)
		{
			STACK_TRACEBACK("Compound Size: [%zu]", nRoot.GetData<NBT_Node::NBT_Compound>().size());
		}
		
		return iRet;//���ݷ���ֵ
	}
public:
	NBT_Reader(void) = delete;
	~NBT_Reader(void) = delete;

	//szStackDepth ����ջ��ȣ��ݹ������ɿ�Ƕ�׵Ŀ��ܽ��еݹ�ĺ������У�ջ��ȵݼ����ɶ�ѡ�����ĵ��ý���
	static bool ReadNBT(NBT_Node &nRoot, const DataType &tData, size_t szDataStartIndex = 0, size_t szStackDepth = 512) noexcept//��data�ж�ȡnbt
	{
		MYTRY
		//��ʼ��NBT������
		nRoot.Clear();//���ԭ�������ݣ�ע�����nbt�ϴ������£�����һ������ĵݹ�������̣����ų�ջ�ռ䲻�㵼������ʧ�ܣ��������׳��쳣֮��ģ���Ҫtry catch
		nRoot.emplace<NBT_Node::NBT_Compound>();//����Ϊcompound

		//��ʼ������������
		InputStream IptStream{ tData,szDataStartIndex };

		//������ջ���
		printf("Max Stack Depth [%zu]\n", szStackDepth);

		//��ʼ�ݹ��ȡ
		return GetNBT<true>(IptStream, nRoot, szStackDepth) == AllOk;//��data�л�ȡnbt���ݵ�nRoot�У�ֻ�д˵���Ϊ�������ã�ģ��true�������ڴ����������
		MYCATCH_OTHER//���������쳣
	}


#undef MYTRY
#undef MYCATCH
#undef CHECK_STACK_DEPTH
#undef STACK_TRACEBACK
#undef STRLING
#undef _RP_STRLING
#undef _RP___LINE__
#undef _RP___FUNCSIG__
};