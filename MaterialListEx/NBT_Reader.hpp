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
	enum ErrCode : uint8_t
	{
		AllOk = 0,//û������

		UnknownError,//��������
		StdException,//��׼�쳣
		OutOfMemoryError,//�ڴ治�����NBT�ļ����⣩
		ListElementTypeError,//�б�Ԫ�����ʹ���NBT�ļ����⣩
		StackDepthExceeded,//����ջ��ȹ��NBT�ļ�or�����������⣩
		NbtTypeTagError,//NBT��ǩ���ʹ���NBT�ļ����⣩
		OutOfRangeError,//��NBT�ڲ����ȴ����������NBT�ļ����⣩
		InternalTypeError,//����NBT�ڵ����ʹ��󣨴������⣩

		ERRCODE_END,//������ǣ�ͳ�Ƹ������ִ�С
	};

	constexpr static inline const char *const errReason[] =
	{
		"AllOk",

		"UnknownError",
		"StdException",
		"OutOfMemoryError",
		"ListElementTypeError",
		"StackDepthExceeded",
		"NbtTypeTagError",
		"OutOfRangeError",
		"InternalTypeError",
	};

	//�ǵ�ͬ�����飡
	static_assert(sizeof(errReason) / sizeof(errReason[0]) == ERRCODE_END, "errReason array out sync");

	enum WarnCode : uint8_t
	{
		NoWarn = 0,

		ElementExistsWarn,

		WARNCODE_END,
	};

	constexpr static inline const char *const warnReason[] =//�������飬ֱ����WarnCode����
	{
		"NoWarn",

		"ElementExistsWarn",
	};

	//�ǵ�ͬ�����飡
	static_assert(sizeof(warnReason) / sizeof(warnReason[0]) == WARNCODE_END, "warnReason array out sync");

	//error����
	//ʹ�ñ���βα�+vprintf���������������������չ��Ϣ
	//������������Ĵ�����������eRet = Error���棬Ȼ�󴥷�STACK_TRACEBACK����󷵻�eRet����һ��
	//��һ�����صĴ���ͨ��if (eRet != AllOk)�жϵģ�ֱ�Ӵ���STACK_TRACEBACK�󷵻�eRet����һ��
	//����Ǿ���ֵ���򲻷���ֵ
	template <typename T>
	requires(std::is_same_v<T, ErrCode> || std::is_same_v<T, WarnCode>)
	static std::conditional_t<std::is_same_v<T, ErrCode>, ErrCode, void> _cdecl Error(const T &code, const InputStream &tData, _Printf_format_string_ const char *const cpExtraInfo = NULL, ...) noexcept//gccʹ��__attribute__((format))��msvcʹ��_Printf_format_string_
	{
		if constexpr (std::is_same_v<T, ErrCode>)
		{
			if (code >= ERRCODE_END)
			{
				return code;
			}
			//�Ϸ�if��֤code�������
			printf("Read Err[%d]: \"%s\"\n", code, errReason[code]);
		}
		else if constexpr (std::is_same_v<T, WarnCode>)
		{
			if (code >= WARNCODE_END)
			{
				return;
			}
			//�Ϸ�if��֤code�������
			printf("Read Warn[%d]: \"%s\"\n", code, warnReason[code]);
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
		//�����Ϣ
		printf
		(
			"Data Review:\n"\
			"Current: 0x%02llX(%zu)\n"\
			"Data Size: 0x%02llX(%zu)\n"\
			"Data[0x%02llX(%zu)]~[0x%02llX(%zu)]:\n",

			(uint64_t)tData.Index(), tData.Index(),
			(uint64_t)tData.Size(), tData.Size(),
			(uint64_t)rangeBeg, rangeBeg,
			(uint64_t)rangeEnd - 1LLU, rangeEnd - 1
		);
		
		//������
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

		//�����ʾ��Ϣ
		if constexpr (std::is_same_v<T, ErrCode>)
		{
			printf("\nSkip err data and return...\n\n");
		}
		else if constexpr (std::is_same_v<T, WarnCode>)
		{
			printf("\nSkip warn data and continue...\n\n");
		}
		else
		{
			static_assert(false, "Unknown [T code] Type!");
		}

		//���治����ֵ
		if constexpr (std::is_same_v<T, ErrCode>)
		{
			return code;
		}
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
		eRet = Error(StackDepthExceeded, tData, _RP___FUNCSIG__ ": NBT nesting depth exceeded maximum call stack limit");\
		STACK_TRACEBACK("(Depth) <= 0");\
		return eRet;\
	}\
}

#define MYTRY \
try\
{

#define MYCATCH \
}\
catch(const std::bad_alloc &e)\
{\
	ErrCode eRet = Error(OutOfMemoryError, tData, _RP___FUNCSIG__ ": Info:[%s]", e.what());\
	STACK_TRACEBACK("catch(std::bad_alloc)");\
	return eRet;\
}\
catch(const std::exception &e)\
{\
	ErrCode eRet = Error(StdException, tData, _RP___FUNCSIG__ ": Info:[%s]", e.what());\
	STACK_TRACEBACK("catch(std::exception)");\
	return eRet;\
}\
catch(...)\
{\
	ErrCode eRet =  Error(UnknownError, tData, _RP___FUNCSIG__ ": Info:[Unknown Exception]");\
	STACK_TRACEBACK("catch(...)");\
	return eRet;\
}


	// ��ȡ�������ֵ��bNoCheckΪtrue�򲻽����κμ��
	template<bool bNoCheck = false, typename T>
	static std::conditional_t<bNoCheck, void, ErrCode> ReadBigEndian(InputStream &tData, T &tVal) noexcept
	{
		if constexpr (!bNoCheck)
		{
			if (!tData.HasAvailData(sizeof(T)))
			{
				ErrCode eRet = Error(OutOfRangeError, tData, "tData size [%zu], current index [%zu], remaining data size [%zu], but try to read [%zu]",
					tData.Size(), tData.Index(), tData.Size() - tData.Index(), sizeof(T));
				STACK_TRACEBACK("HasAvailData");
				return eRet;
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

	static ErrCode GetName(InputStream &tData, NBT_Type::String &tNameRet) noexcept
	{
	MYTRY;
		ErrCode eRet = AllOk;
		//��ȡ2�ֽڵ��޷������Ƴ���
		NBT_Type::StringLength wStringLength = 0;//w->word=2*byte
		eRet = ReadBigEndian(tData, wStringLength);
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("wStringLength Read");
			return eRet;
		}

		//�жϳ����Ƿ񳬹�
		if (!tData.HasAvailData(wStringLength))
		{
			ErrCode eRet = Error(OutOfRangeError, tData, __FUNCSIG__ ": (Index[%zu] + wStringLength[%zu])[%zu] > DataSize[%zu]",
				tData.Index(), (size_t)wStringLength, tData.Index() + (size_t)wStringLength, tData.Size());
			STACK_TRACEBACK("HasAvailData");
			return eRet;
		}

		
		//����������
		tNameRet.reserve(wStringLength);//��ǰ����
		tNameRet.assign(tData.Current(), tData.Next(wStringLength));//����string���������Ϊ0����0���ַ������Ϸ���Ϊ��
		tData.AddIndex(wStringLength);//�ƶ��±�

		return eRet;
	MYCATCH;
	}

	template<typename T>
	static ErrCode GetBuiltInType(InputStream &tData, T &tBuiltInRet) noexcept
	{
		ErrCode eRet = AllOk;

		//��ȡ����
		using RAW_DATA_T = NBT_Type::BuiltinRawType_T<T>;//����ӳ��

		//��ʱ�洢����Ϊ���ܴ��ڿ�����ת��
		RAW_DATA_T tTmpRawData = 0;
		eRet = ReadBigEndian(tData, tTmpRawData);
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("tTmpRawData Read");
			return eRet;
		}

		//ת��������
		tBuiltInRet = std::move(std::bit_cast<T>(tTmpRawData));
		return eRet;
	}

	template<typename T>
	static ErrCode GetArrayType(InputStream &tData, T &tArrayRet) noexcept
	{
	MYTRY;
		ErrCode eRet = AllOk;

		//��ȡ4�ֽ��з���������������Ԫ�ظ���
		NBT_Type::ArrayLength iElementCount = 0;//4byte
		eRet = ReadBigEndian(tData, iElementCount);
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("iElementCount Read");
			return eRet;
		}

		//�жϳ����Ƿ񳬹�
		if (!tData.HasAvailData(iElementCount * sizeof(T::value_type)))//��֤�·����ð�ȫ
		{
			eRet = Error(OutOfRangeError, tData, __FUNCSIG__ ": (Index[%zu] + iElementCount[%zu] * sizeof(T::value_type)[%zu])[%zu] > DataSize[%zu]", 
				tData.Index(), (size_t)iElementCount, sizeof(T::value_type), tData.Index() + (size_t)iElementCount * sizeof(T::value_type), tData.Size());
			STACK_TRACEBACK("HasAvailData Test");
			return eRet;
		}
		
		//���鱣��
		tArrayRet.reserve(iElementCount);//��ǰ����
		//��ȡdElementCount��Ԫ��
		for (NBT_Type::ArrayLength i = 0; i < iElementCount; ++i)
		{
			typename T::value_type tTmpData{};
			ReadBigEndian<true>(tData, tTmpData);//������Ҫȷ����Χ��ȫ
			tArrayRet.emplace_back(std::move(tTmpData));//��ȡһ������һ��
		}

		return eRet;
	MYCATCH;
	}

	//����ǷǸ������ж�����
	template<bool bRoot>
	static ErrCode GetCompoundType(InputStream &tData, NBT_Type::Compound &tCompoundRet, size_t szStackDepth) noexcept
	{
	MYTRY;
		ErrCode eRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);

		//��ȡ
		while (true)
		{
			//����ĩβ���
			if (!tData.HasAvailData(sizeof(NBT_TAG_RAW_TYPE)))
			{
				if constexpr (!bRoot)//�Ǹ����������ĩβ���򱨴�
				{
					eRet = Error(OutOfRangeError, tData, __FUNCSIG__ ": Index[%zu] >= DataSize()[%zu]", tData.Index(), tData.Size());
				}

				return eRet;//����ֱ�ӷ��أ�Ĭ��ֵAllOk��
			}

			//�ȶ�ȡһ������
			NBT_TAG tagNbt = (NBT_TAG)(NBT_TAG_RAW_TYPE)tData.GetNext();
			if (tagNbt == NBT_TAG::End)//����End���
			{
				return eRet;//ֱ�ӷ��أ�Ĭ��ֵAllOk��
			}

			if (tagNbt >= NBT_TAG::ENUM_END)//ȷ���ڷ�Χ��
			{
				eRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": NBT Tag switch default: Unknown Type Tag[%02X(%d)]", tagNbt, tagNbt);//�˴���������ǰ���أ�����Ĭ�Ϸ��ش���
				STACK_TRACEBACK("tagNbt >= NBT_TAG::ENUM_END");
				return eRet;//������Χ���̷���
			}

			//Ȼ���ȡ����
			NBT_Type::String sName{};
			eRet = GetName(tData, sName);
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("GetName Fail, Name: \"\" [NBT_Type::%s]", NBT_Type::GetTypeName(tagNbt));
				return eRet;//���ƶ�ȡʧ�����̷���
			}

			//Ȼ��������ͣ����ö�Ӧ�����Ͷ�ȡ�����ص�tmpNode
			NBT_Node tmpNode{};
			eRet = GetSwitch(tData, tmpNode, tagNbt, szStackDepth - 1);
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("GetSwitch Fail, Name: \"%s\" [NBT_Type::%s]", U16ANSI(U16STR(sName)).c_str(), NBT_Type::GetTypeName(tagNbt));
				//return eRet;//ע��˴������أ����в��룬�Ա��������֮ǰ����ȷ����
			}

			//sName:tmpNode�����뵱ǰ����ջ��ȵĸ��ڵ�
			//����ʵ��mc java����ó����������һ���Ѿ����ڵļ����ᵼ��ԭ�ȵ�ֵ���滻������
			//��ô��ʧ�ܺ��ֶ��ӵ������滻��ǰֵ��ע�⣬�˴�������try_emplace����Ϊtry_emplaceʧ�ܺ�ԭ�ȵ�ֵ
			//tmpNode���ᱻ�ƶ����¶�ʧ������Ҳ���追�������Է�ֹ�ƶ���ʧ����
			auto [it,bSuccess] = tCompoundRet.try_emplace(std::move(sName), std::move(tmpNode));
			if (!bSuccess)
			{
				//ʹ�õ�ǰֵ�滻����ֹ�����ԭʼֵ
				it->second = std::move(tmpNode);

				//�������棬ע�⾯�治��eRet�ӷ���ֵ
				Error(ElementExistsWarn, tData, __FUNCSIG__ ": Name: \"%s\" [NBT_Type::%s] tData already exist!",
					U16ANSI(U16STR(sName)).c_str(), NBT_Type::GetTypeName(tagNbt));
			}

			//����ж��Ƿ����
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("While break with an error!");
				return eRet;//������
			}
		}

		return eRet;//���ش�����
	MYCATCH;
	}

	static ErrCode GetStringType(InputStream &tData, NBT_Type::String &tStringRet) noexcept
	{
		ErrCode eRet = AllOk;

		//��ȡ�ַ���
		eRet = GetName(tData, tStringRet);//��Ϊstring��name��ȡԭ��һ�£�ֱ�ӽ���ʵ��
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("GetString");//����СС�ĸĸ�������ֹ��������
			return eRet;
		}

		return eRet;
	}

	static ErrCode GetListType(InputStream &tData, NBT_Type::List &tListRet, size_t szStackDepth) noexcept
	{
	MYTRY;
		ErrCode eRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);

		//��ȡ1�ֽڵ��б�Ԫ������
		NBT_TAG_RAW_TYPE bListElementType = 0;//b=byte
		eRet = ReadBigEndian(tData, bListElementType);
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("bListElementType Read");
			return eRet;
		}

		//������б�Ԫ������
		if (bListElementType >= NBT_TAG::ENUM_END)
		{
			eRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": List NBT Type:Unknown Type Tag[%02X(%d)]", bListElementType, bListElementType);
			STACK_TRACEBACK("bListElementType Test");
			return eRet;
		}

		//��ȡ4�ֽڵ��з����б���
		NBT_Type::ListLength iListLength = 0;//4byte
		eRet = ReadBigEndian(tData, iListLength);
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("iListLength Read");
			return eRet;
		}

		//����з�������С��Χ
		if (iListLength < 0)
		{
			eRet = Error(OutOfRangeError, tData, __FUNCSIG__ ": iListLength[%d] < 0", iListLength);
			STACK_TRACEBACK("iListLength Test");
			return eRet;
		}

		//��ֹ�ظ�N��������ǩ�����н�����ǩ�ı����ǿ��б�
		if (bListElementType == NBT_TAG::End && iListLength != 0)
		{
			eRet = Error(ListElementTypeError, tData, __FUNCSIG__ ": The list with TAG_End[0x00] tag must be empty, but [%d] elements were found", iListLength);
			STACK_TRACEBACK("bListElementType And iListLength Test");
			return eRet;
		}

		//ȷ���������Ϊ0������£��б����ͱ�ΪEnd
		if (iListLength == 0 && bListElementType != NBT_TAG::End)
		{
			bListElementType = (NBT_TAG_RAW_TYPE)NBT_TAG::End;
		}

		//�������Ͳ���ǰ����
		tListRet.enElementTag = (NBT_TAG)bListElementType;//����������
		tListRet.reserve(iListLength);//��֪��С��ǰ������ٿ���

		//����Ԫ�����ͣ���ȡn���б�
		for (NBT_Type::ListLength i = 0; i < iListLength; ++i)
		{
			NBT_Node tmpNode{};//�б�Ԫ�ػ�ֱ�Ӹ�ֵ�޸�
			eRet = GetSwitch(tData, tmpNode, (NBT_TAG)bListElementType, szStackDepth - 1);
			if (eRet != AllOk)//������
			{
				STACK_TRACEBACK("Size: [%d] Index: [%d]", iListLength, i);
				return eRet;
			}

			//ÿ��ȡһ���������һ��
			tListRet.emplace_back(std::move(tmpNode));
		}
		
		return eRet;
	MYCATCH;
	}

	//����������������ڲ����ò������쳣�������أ����Դ˺������Բ��׳��쳣���ɴ˵��ô˺����ĺ���Ҳ������catch�쳣
	static ErrCode GetSwitch(InputStream &tData, NBT_Node &nodeRet, NBT_TAG tagNbt, size_t szStackDepth) noexcept//ѡ���������ݹ�㣬�ɺ������õĺ������
	{
		ErrCode eRet = AllOk;

		switch (tagNbt)
		{
		case NBT_TAG::Byte:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Byte>;
				nodeRet.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeRet.GetData<CurType>());
			}
			break;
		case NBT_TAG::Short:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Short>;
				nodeRet.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeRet.GetData<CurType>());
			}
			break;
		case NBT_TAG::Int:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Int>;
				nodeRet.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeRet.GetData<CurType>());
			}
			break;
		case NBT_TAG::Long:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Long>;
				nodeRet.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeRet.GetData<CurType>());
			}
			break;
		case NBT_TAG::Float:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Float>;
				nodeRet.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeRet.GetData<CurType>());
			}
			break;
		case NBT_TAG::Double:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Double>;
				nodeRet.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeRet.GetData<CurType>());
			}
			break;
		case NBT_TAG::ByteArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::ByteArray>;
				nodeRet.emplace<CurType>();
				eRet = GetArrayType<CurType>(tData, nodeRet.GetData<CurType>());
			}
			break;
		case NBT_TAG::String:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::String>;
				nodeRet.emplace<CurType>();
				eRet = GetStringType(tData, nodeRet.GetData<CurType>());
			}
			break;
		case NBT_TAG::List://��Ҫ�ݹ���ã��б�ͷ������ǩID�ͳ��ȣ�������Ϊһϵ��ͬ���ͱ�ǩ����Ч���أ��ޱ�ǩ ID �����ƣ�
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::List>;
				nodeRet.emplace<CurType>();
				eRet = GetListType(tData, nodeRet.GetData<CurType>(), szStackDepth);//ѡ���������ٵݹ��
			}
			break;
		case NBT_TAG::Compound://��Ҫ�ݹ����
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Compound>;
				nodeRet.emplace<CurType>();
				eRet = GetCompoundType<false>(tData, nodeRet.GetData<CurType>(), szStackDepth);//ѡ���������ٵݹ��
			}
			break;
		case NBT_TAG::IntArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::IntArray>;
				nodeRet.emplace<CurType>();
				eRet = GetArrayType<CurType>(tData, nodeRet.GetData<CurType>());
			}
			break;
		case NBT_TAG::LongArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::LongArray>;
				nodeRet.emplace<CurType>();
				eRet = GetArrayType<CurType>(tData, nodeRet.GetData<CurType>());
			}
			break;
		case NBT_TAG::End://��Ӧ�����κ�ʱ�������˱�ǩ��Compound���ȡ�������ĵ������ᴫ�룬List�����˱�ǩ������ö�ȡ������������Ϊ����
		default://����δ֪��ǩ
			{//NBT�ڱ�����ǩ����
				eRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": NBT Tag switch error: Unknown or unexpected Type Tag[%02X(%d)]", tagNbt, tagNbt);//�˴���������ǰ���أ�����Ĭ�Ϸ��ش���
			}
			break;
		}
		
		if (eRet != AllOk)//���������һ��ջ����
		{
			STACK_TRACEBACK("Tag read error!");
		}

		return eRet;//���ݷ���ֵ
	}

public:
	NBT_Reader(void) = delete;
	~NBT_Reader(void) = delete;

	/*
	��ע���˺�����ȡnbtʱ���ᴴ��һ��Ĭ�ϸ���Ȼ���nbt���������ݼ��ϵ���Ĭ�ϸ��ϣ�
	Ҳ�������°���mojang��nbt��׼��Ĭ�ϸ�������compound��Ҳ�ᱻ�ҽӵ�����ֵ���
	�ں�NBT_Type::Compound��NBT_Node�У�Ȼ�����NBT_Node�������ɵõ����и���
	��ô����Ŀ����Ϊ�˷����д�����Ҳ������ⲿ����ʵ��mojang������compound���⴦��

	����������һ���̶���������mojang��׼֧�ָ����NBT�ļ������
	�����ļ��ڲ�����Compound��ʼ�ģ����ǵ����ļ�����ͬ�����Ҵ������ֵ�NBT����ôҲ��
	������ȡ����ȫ������NBT_Type::Compound�У��ͺ���nbt�ļ��������һ��
	NBT_Type::Compoundһ������Եģ�д�뺯��Ҳ��֧��д��
	
	���Զ��ڶԳƺ���WriteNBTд����ʱ�򣬴����ֵҲ��һ���ں�NBT_Type::Compound��
	NBT_Node��Ȼ�����NBT_Type::Compound�����ᱻ���κ���ʽд��NBT�ļ�������
	�ҽ������������д�룬�������ܱ�֤����mojang��nbt�ļ���Ҳ��һ���̶�����չnbt�ļ�
	�ڿ��Դ洢�����ݣ�Ҳ���ǲ������Ƿ�Ҫ��һ�������Ƶ�compound��ͷ��
	*/

	//szStackDepth ����ջ��ȣ��ݹ������ɿ�Ƕ�׵Ŀ��ܽ��еݹ�ĺ������У�ջ��ȵݼ����ɶ�ѡ�����ĵ��ý���
	static bool ReadNBT(NBT_Node &nRoot, const DataType &tData, size_t szDataStartIndex = 0, size_t szStackDepth = 512) noexcept//��data�ж�ȡnbt
	{
	MYTRY;
		//��ʼ��NBT������
		nRoot.emplace<NBT_Type::Compound>();//����Ϊcompound

		//��ʼ������������
		InputStream IptStream(tData, szDataStartIndex);

		//������ջ���
		printf("Max Stack Depth [%zu]\n", szStackDepth);

		//��ʼ�ݹ��ȡ
		return GetCompoundType<true>(IptStream, nRoot.GetCompound(), szStackDepth) == AllOk;//��data�л�ȡnbt���ݵ�nRoot�У�ֻ�д˵���Ϊ�������ã�ģ��true�������ڴ����������
	MYCATCH;
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