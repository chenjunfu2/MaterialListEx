#pragma once

#include <new>//std::bad_alloc
#include <bit>//std::bit_cast

#include "NBT_Node.hpp"

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

template <typename DataType = std::basic_string<NBT_TAG_RAW_TYPE>>
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

	//error����
	//ʹ�ñ���βα�+vprintf���������������������չ��Ϣ
	/*
		������������Ĵ�����������iRet = Error���棬Ȼ�󴥷�STACK_TRACEBACK����󷵻�iRet����һ��
		��һ�����صĴ���ͨ��if (iRet < AllOk)�жϵģ�ֱ�Ӵ���STACK_TRACEBACK�󷵻�iRet����һ��
	*/
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
				printf(" %02X ", (NBT_TAG_RAW_TYPE)tData[i]);
			}
			else//����ǵ�ǰ�����ֽڣ��ӷ����ſ���
			{
				printf("[%02X]", (NBT_TAG_RAW_TYPE)tData[i]);
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
			tVal = (T)(NBT_TAG_RAW_TYPE)tData.GetNext();
		}
		else
		{
			T tTmp = 0;
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				tTmp <<= 8;
				tTmp |= (T)(NBT_TAG_RAW_TYPE)tData.GetNext();//��Ϊֻ�����ƣ��������з��ŵ��µ�����λ��bug������ת��Ϊ�޷�������
			}
			tVal = tTmp;
		}

		if constexpr (!bNoCheck)
		{
			return AllOk;
		}
	}

	static inline int GetName(InputStream &tData, NBT_Type::String &tNameRet)
	{
		int iRet = AllOk;
		//��ȡ2�ֽڵ��޷������Ƴ���
		NBT_Type::StringLength wStringLength = 0;//w->word=2*byte
		iRet = ReadBigEndian(tData, wStringLength);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("wStringLength Read");
			return iRet;
		}

		//�жϳ����Ƿ񳬹�
		if (!tData.HasAvailData(wStringLength))
		{
			int iRet = Error(OutOfRangeError, tData, __FUNCSIG__ ": (Index[%zu] + wStringLength[%zu])[%zu] > DataSize[%zu]",
				tData.Index(), (size_t)wStringLength, tData.Index() + (size_t)wStringLength, tData.Size());
			STACK_TRACEBACK("HasAvailData");
			return iRet;
		}

		MYTRY;
		//����������
		tNameRet.reserve(wStringLength);//��ǰ����
		tNameRet.assign(tData.Current(), tData.Next(wStringLength));//�������Ϊ0����0���ַ������Ϸ���Ϊ
		MYCATCH_BADALLOC;

		tData.AddIndex(wStringLength);//�ƶ��±�
		return AllOk;
	}

	template<typename T>
	static inline int GetBuiltInType(InputStream &tData, T &tBuiltInRet)
	{
		int iRet = AllOk;

		//��ȡ����
		using RAW_DATA_T = NBT_Type::BuiltinRawType_T<T>;//����ӳ��

		//��ʱ�洢����Ϊ���ܴ��ڿ�����ת��
		RAW_DATA_T tTmpRawData = 0;
		iRet = ReadBigEndian(tData, tTmpRawData);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("tTmpRawData Read");
			return iRet;
		}

		//ת��������
		tBuiltInRet = std::move(std::bit_cast<T>(tTmpRawData));
		return iRet;
	}

	template<typename T>
	static inline int GetArrayType(InputStream &tData, T &tArrayRet)
	{
		int iRet = AllOk;

		//��ȡ4�ֽ��з���������������Ԫ�ظ���
		NBT_Type::ArrayLength iElementCount = 0;//4byte
		iRet = ReadBigEndian(tData, iElementCount);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("iElementCount Read");
			return iRet;
		}

		//�жϳ����Ƿ񳬹�
		if (!tData.HasAvailData(iElementCount * sizeof(T::value_type)))//��֤�·����ð�ȫ
		{
			iRet = Error(OutOfRangeError, tData, __FUNCSIG__ ": (Index[%zu] + iElementCount[%zu] * sizeof(T::value_type)[%zu])[%zu] > DataSize[%zu]", 
				tData.Index(), (size_t)iElementCount, sizeof(T::value_type), tData.Index() + (size_t)iElementCount * sizeof(T::value_type), tData.Size());
			STACK_TRACEBACK("HasAvailData Test");
			return iRet;
		}
		
		//���鱣��
		MYTRY;
		tArrayRet.reserve(iElementCount);//��ǰ����
		//��ȡdElementCount��Ԫ��
		for (NBT_Type::ArrayLength i = 0; i < iElementCount; ++i)
		{
			typename T::value_type tTmpData;
			ReadBigEndian<true>(tData, tTmpData);//������Ҫȷ����Χ��ȫ
			tArrayRet.emplace_back(std::move(tTmpData));//��ȡһ������һ��
		}
		MYCATCH_BADALLOC;

		return iRet;
	}

	static int GetCompoundType(InputStream &tData, NBT_Type::Compound &tCompoundRet, size_t szStackDepth)//�˵ݹ麯����Ҫ��֤���ݷ���ֵ
	{
		int iRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);

		//��ʼ�ݹ�
		//�˴����貶���쳣����ΪGetNBT�ڲ����õ�switchNBT�����׳��쳣
		iRet = GetNBT(tData, tCompoundRet, szStackDepth);//�˴�ֱ�Ӵ��ݣ���ΪGetNBT�ڲ�����еݼ�
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("GetNBT");
			//return iRet;//�����������в��룬�Ա��������֮ǰ����ȷ����
		}

		//>=AllOkǿ�Ʒ���AllOk�Է�ֹCompound_End
		return iRet >= AllOk ? AllOk : iRet;
	}

	static inline int GetStringType(InputStream &tData, NBT_Type::String &tStringRet)
	{
		int iRet = AllOk;

		//��ȡ�ַ���
		iRet = GetName(tData, tStringRet);//��Ϊstring��name��ȡԭ��һ�£�ֱ�ӽ���ʵ��
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("GetName");
			return iRet;
		}

		return iRet;
	}

	static int GetListType(InputStream &tData, NBT_Type::List &tListRet, size_t szStackDepth)
	{
		int iRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);

		//��ȡ1�ֽڵ��б�Ԫ������
		NBT_TAG_RAW_TYPE bListElementType = 0;//b=byte
		iRet = ReadBigEndian(tData, bListElementType);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("bListElementType Read");
			return iRet;
		}

		//������б�Ԫ������
		if (bListElementType >= NBT_TAG::ENUM_END)
		{
			iRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": List NBT Type:Unknown Type Tag[%02X(%d)]", bListElementType, bListElementType);
			STACK_TRACEBACK("bListElementType Test");
			return iRet;
		}

		//��ȡ4�ֽڵ��з����б���
		NBT_Type::ListLength iListLength = 0;//4byte
		iRet = ReadBigEndian(tData, iListLength);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("iListLength Read");
			return iRet;
		}

		//����з�������С��Χ
		if (iListLength < 0)
		{
			iRet = Error(OutOfRangeError, tData, __FUNCSIG__ ": iListLength[%d] < 0", iListLength);
			STACK_TRACEBACK("iListLength Test");
			return iRet;
		}

		//��ֹ�ظ�N��������ǩ�����н�����ǩ�ı����ǿ��б�
		if (bListElementType == NBT_TAG::End && iListLength != 0)
		{
			iRet = Error(ListElementTypeError, tData, __FUNCSIG__ ": The list with TAG_End[0x00] tag must be empty, but [%d] elements were found", iListLength);
			STACK_TRACEBACK("bListElementType And iListLength Test");
			return iRet;
		}

		//ȷ���������Ϊ0������£��б����ͱ�ΪEnd
		if (iListLength == 0 && bListElementType != NBT_TAG::End)
		{
			bListElementType = (NBT_TAG_RAW_TYPE)NBT_TAG::End;
		}

		//����Ԫ�����ͣ���ȡn���б�
		tListRet.enElementTag = (NBT_TAG)bListElementType;//����������
		MYTRY;
		tListRet.reserve(iListLength);//��֪��С��ǰ������ٿ���

		for (NBT_Type::ListLength i = 0; i < iListLength; ++i)
		{
			NBT_Node tmpNode{};//�б�Ԫ�ػ�ֱ�Ӹ�ֵ�޸�
			iRet = SwitchNBT<false>(tData, tmpNode, (NBT_TAG)bListElementType, szStackDepth - 1);
			if (iRet < AllOk)//������
			{
				STACK_TRACEBACK("Size: [%d] Index: [%d]", iListLength, i);
				break;//����ѭ���Ա�����������֮ǰ����ȷ����
			}

			//ÿ��ȡһ���������һ��
			tListRet.emplace_back(std::move(tmpNode));
		}
		MYCATCH_BADALLOC;
		
		//��������򴫵ݣ����򷵻�Ok
		return iRet >= AllOk ? AllOk : iRet;//�б�ͬʱ��ΪԪ�أ��ɹ�Ӧ�÷���Ok�������Ǵ��ݷ���ֵ
	}

	//����������������ڲ����ò������쳣�������أ����Դ˺������Բ��׳��쳣���ɴ˵��ô˺����ĺ���Ҳ������catch�쳣
	template<bool bHasName>//���������б��б�Ԫ�������ƣ��Һ��������ı�
	static int SwitchNBT(InputStream &tData, std::conditional_t<bHasName, NBT_Type::Compound, NBT_Node> &nRoot, NBT_TAG tag, size_t szStackDepth) noexcept//ѡ���������ݹ�㣬�ɺ������õĺ������
	{
		int iRet = AllOk;
		
#define GETNAME\
		/*��ȡNBT��N�����ƣ�*/\
		NBT_Type::String sName{};\
		if constexpr (bHasName)/*�����֣����б�Ԫ�أ����ȡ*/\
		{\
			iRet = GetName(tData, sName);\
			if (iRet < AllOk)\
			{\
				STACK_TRACEBACK("GetName");\
				return iRet;\
			}\
		}

#define ADDROOT\
		if constexpr (bHasName)/*��������ǰ�Ǽ���Compound�ڵ㣬��nRoot��������*/\
		{\
			MYTRY;\
			/*���� - �ں����ݵĽڵ���뵱ǰ����ջ��ȵĸ��ڵ�*/\
			auto ret = nRoot.try_emplace(std::move(sName), std::move(tmpNode));\
			if (!ret.second)/*����ʧ�ܣ�Ԫ���Ѵ��ڣ�ע�⾯�治����errorֵ*/\
			{\
				Error(ElementExistsWarn, tData, __FUNCSIG__ ": the \"%s\"[NBT_Type::%s] tData already exist!",\
				U16ANSI(U16STR(sName)).c_str(), NBT_Type::GetTypeName(tag));\
			}\
			MYCATCH_BADALLOC;\
		}\
		else/*��������ǰ���б�Ԫ�أ�ֱ���޸�nRoot��Ȼ�󷵻ظ��б�*/\
		{\
			if constexpr(std::is_same_v<CurType, NBT_Node>)/*�����NBT_Nodeֱ�Ӹ�ֵ*/\
			{\
				nRoot = std::move(tmpNode);\
			}\
			else/*�����쵽node��*/\
			{\
				nRoot.emplace<CurType>(tmpNode); \
			}\
		}

		MYTRY;
		switch (tag)
		{
		case NBT_TAG::End:
			{
				iRet = Compound_End;//end�������޸��أ�ֱ�����÷���ֵ
			}
			break;
		case NBT_TAG::Byte:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Byte>;
				GETNAME;
				CurType tmpNode{};
				iRet = GetBuiltInType<CurType>(tData, tmpNode);
				ADDROOT;
			}
			break;
		case NBT_TAG::Short:
			{
				GETNAME;
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Short>;
				CurType tmpNode{};
				iRet = GetBuiltInType<CurType>(tData, tmpNode);
				ADDROOT;
			}
			break;
		case NBT_TAG::Int:
			{
				GETNAME;
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Int>;
				CurType tmpNode{};
				iRet = GetBuiltInType<CurType>(tData, tmpNode);
				ADDROOT;
			}
			break;
		case NBT_TAG::Long:
			{
				GETNAME;
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Long>;
				CurType tmpNode{};
				iRet = GetBuiltInType<CurType>(tData, tmpNode);
				ADDROOT;
			}
			break;
		case NBT_TAG::Float:
			{
				GETNAME;
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Float>;
				CurType tmpNode{};
				iRet = GetBuiltInType<CurType>(tData, tmpNode);
				ADDROOT;
			}
			break;
		case NBT_TAG::Double:
			{
				GETNAME;
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Double>;
				CurType tmpNode{};
				iRet = GetBuiltInType<CurType>(tData, tmpNode);
				ADDROOT;
			}
			break;
		case NBT_TAG::Byte_Array:
			{
				GETNAME;
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Byte_Array>;
				CurType tmpNode{};
				iRet = GetArrayType<CurType>(tData, tmpNode);
				ADDROOT;
			}
			break;
		case NBT_TAG::String:
			{
				GETNAME;
				using CurType = NBT_Type::TagToType_T<NBT_TAG::String>;
				CurType tmpNode{};
				iRet = GetStringType(tData, tmpNode);
				ADDROOT;
			}
			break;
		case NBT_TAG::List://��Ҫ�ݹ���ã��б�ͷ������ǩID�ͳ��ȣ�������Ϊһϵ��ͬ���ͱ�ǩ����Ч���أ��ޱ�ǩ ID �����ƣ�
			{
				GETNAME;
				using CurType = NBT_Type::TagToType_T<NBT_TAG::List>;
				CurType tmpNode{};
				iRet = GetListType(tData, tmpNode, szStackDepth);//ѡ���������ٵݹ��
				ADDROOT;
			}
			break;
		case NBT_TAG::Compound://��Ҫ�ݹ����
			{
				GETNAME;
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Compound>;
				CurType tmpNode{};
				iRet = GetCompoundType(tData, tmpNode, szStackDepth);//ѡ���������ٵݹ��
				ADDROOT;
			}
			break;
		case NBT_TAG::Int_Array:
			{
				GETNAME;
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Int_Array>;
				CurType tmpNode{};
				iRet = GetArrayType<CurType>(tData, tmpNode);
				ADDROOT;
			}
			break;
		case NBT_TAG::Long_Array:
			{
				GETNAME;
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Long_Array>;
				CurType tmpNode{};
				iRet = GetArrayType<CurType>(tData, tmpNode);
				ADDROOT;
			}
			break;
		default://NBT�ڱ�����ǩ����
			{
				iRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": NBT Tag switch default: Unknown Type Tag[%02X(%d)]", tag, tag);//�˴���������ǰ���أ�����Ĭ�Ϸ��ش���
			}
			break;
		}
		MYCATCH_OTHER;//���ﲶ����������δ������쳣

		//����Ѿ������쳣��ֱ�ӷ��أ�����������
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("Tag read error!");
		}

#undef ADDROOT
#undef GETNAME

		return iRet;//���ݷ���ֵ
	}

	//���׳��쳣
	template<bool bRoot = false>//�����ػ��汾�����ڴ���ĩβ
	static int GetNBT(InputStream &tData, NBT_Type::Compound &nRoot, size_t szStackDepth) noexcept//�ݹ���ö�ȡ����ӽڵ�
	{
		int iRet = AllOk;//Ĭ��ֵ
		CHECK_STACK_DEPTH(szStackDepth);
		
		//��ȡ
		do
		{
			if (!tData.HasAvailData(sizeof(NBT_TAG_RAW_TYPE)))//����ĩβ���
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
			iRet = SwitchNBT<true>(tData, nRoot, (NBT_TAG)(NBT_TAG_RAW_TYPE)tData.GetNext(), szStackDepth - 1);//�Ѳ��������쳣
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
			STACK_TRACEBACK("Compound Size: [%zu]", nRoot.size());
		}
		
		return iRet;//���ݷ���ֵ
	}
public:
	NBT_Reader(void) = delete;
	~NBT_Reader(void) = delete;

	//szStackDepth ����ջ��ȣ��ݹ������ɿ�Ƕ�׵Ŀ��ܽ��еݹ�ĺ������У�ջ��ȵݼ����ɶ�ѡ�����ĵ��ý���
	static bool ReadNBT(NBT_Node &nRoot, const DataType &tData, size_t szDataStartIndex = 0, size_t szStackDepth = 512) noexcept//��data�ж�ȡnbt
	{
		MYTRY;
		//��ʼ��NBT������
		nRoot.Clear();//���ԭ�������ݣ�ע�����nbt�ϴ������£�����һ������ĵݹ�������̣����ų�ջ�ռ䲻�㵼������ʧ�ܣ��������׳��쳣֮��ģ���Ҫtry catch
		nRoot.emplace<NBT_Type::Compound>();//����Ϊcompound

		//��ʼ������������
		InputStream IptStream{ tData,szDataStartIndex };

		//������ջ���
		printf("Max Stack Depth [%zu]\n", szStackDepth);

		//��ʼ�ݹ��ȡ
		return GetNBT<true>(IptStream, nRoot.GetCompound(), szStackDepth) == AllOk;//��data�л�ȡnbt���ݵ�nRoot�У�ֻ�д˵���Ϊ�������ã�ģ��true�������ڴ����������
		MYCATCH_OTHER;//���������쳣
	}


#undef MYTRY
#undef MYCATCH_BADALLOC
#undef MYCATCH_OTHER
#undef CHECK_STACK_DEPTH
#undef STACK_TRACEBACK
#undef STRLING
#undef _RP_STRLING
#undef _RP___LINE__
#undef _RP___FUNCSIG__
};