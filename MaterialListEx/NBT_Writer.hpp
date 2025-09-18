#pragma once

#include <new>//bad alloc
#include <string>
#include <stdint.h>
#include <type_traits>

#include "NBT_Node.hpp"

template <typename T = std::basic_string<uint8_t>>
class MyOutputStream
{
private:
	T &tData;
public:
	MyOutputStream(T &_tData, size_t szStartIdx = 0) :tData(_tData)
	{
		tData.resize(szStartIdx);
	}
	~MyOutputStream(void) = default;

	const typename T::value_type &operator[](size_t szIndex) const noexcept
	{
		return tData[szIndex];
	}

	template<typename V>
	requires(std::is_constructible_v<typename T::value_type, V &&>)
	void PutOnce(V &&c)
	{
		tData.push_back(std::forward<V>(c));
	}

	void PutRange(const typename T::value_type *pData, size_t szSize)
	{
		tData.append(pData, szSize);
	}

	void UnPut(void) noexcept
	{
		tData.pop_back();
	}

	size_t Size(void) const noexcept
	{
		return tData.size();
	}

	void Reset(void) noexcept
	{
		tData.clear();
	}

	const T &Data(void) const noexcept
	{
		return tData;
	}

	T &Data(void) noexcept
	{
		return tData;
	}
};


template <typename DataType = std::basic_string<uint8_t>>
class NBT_Writer
{
	NBT_Writer(void) = delete;
	~NBT_Writer(void) = delete;

	using OutputStream = MyOutputStream<DataType>;//������
private:
	enum ErrCode : uint8_t
	{
		AllOk = 0,//û������

		UnknownError,//��������
		StdException,//��׼�쳣
		OutOfMemoryError,//�ڴ治�����
		ListElementTypeError,//�б�Ԫ�����ʹ��󣨴������⣩
		StackDepthExceeded,//����ջ��ȹ���������⣩
		StringTooLongError,//�ַ�����������
		ArrayTooLongError,//�����������
		ListTooLongError,//�б��������
		NbtTypeTagError,//NBT��ǩ���ʹ���

		ERRCODE_END,//�������
	};

	constexpr static inline const char *const errReason[] =
	{
		"AllOk",

		"UnknownError",
		"StdException",
		"OutOfMemoryError",
		"ListElementTypeError",
		"StackDepthExceeded",
		"StringTooLongError",
		"ArrayTooLongError",
		"ListTooLongError",
		"NbtTypeTagError",
	};

	//�ǵ�ͬ�����飡
	static_assert(sizeof(errReason) / sizeof(errReason[0]) == (ERRCODE_END), "errReason array out sync");

	enum WarnCode : uint8_t
	{
		NoWarn = 0,

		EndElementIgnoreWarn,

		WARNCODE_END,
	};

	constexpr static inline const char *const warnReason[] =//�������飬ֱ����WarnCode����
	{
		"NoWarn",

		"EndElementIgnoreWarn",
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
	static std::conditional_t<std::is_same_v<T, ErrCode>, ErrCode, void> _cdecl Error(const T &code, const OutputStream &tData, _Printf_format_string_ const char *const cpExtraInfo = NULL, ...) noexcept//gccʹ��__attribute__((format))��msvcʹ��_Printf_format_string_
	{
		//��ӡ����ԭ��
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

		//��ӡ��չ��Ϣ
		if (cpExtraInfo != NULL)
		{
			printf("Extra Info:\"");
			va_list args;//�䳤�β�
			va_start(args, cpExtraInfo);
			vprintf(cpExtraInfo, args);
			va_end(args);
			printf("\"\n");
		}

		//������ԣ�Ԥ��szCurrentǰn���ַ���������е��߽�
#define VIEW_PRE (8 * 8 + 8)//��ǰ
		size_t rangeBeg = (tData.Size() > VIEW_PRE) ? (tData.Size() - VIEW_PRE) : (0);//�ϱ߽����
		size_t rangeEnd = tData.Size();//�±߽����
#undef VIEW_PRE
		//�����Ϣ
		printf
		(
			"Data Review:\n"\
			"Data Size: 0x%02llX(%zu)\n"\
			"Data[0x%02llX(%zu),0x%02llX(%zu)):\n",

			(uint64_t)tData.Size(), tData.Size(),
			(uint64_t)rangeBeg, rangeBeg,
			(uint64_t)rangeEnd, rangeEnd
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

			printf(" %02X ", (uint8_t)tData[i]);
		}

		//�����ʾ��Ϣ
		if constexpr (std::is_same_v<T, ErrCode>)
		{
			printf("\nSkip err and return...\n\n");
		}
		else if constexpr (std::is_same_v<T, WarnCode>)
		{
			printf("\nSkip warn and continue...\n\n");
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
if((Depth) <= 0)\
{\
	eRet = Error(StackDepthExceeded, tData, _RP___FUNCSIG__ ": NBT nesting depth exceeded maximum call stack limit");\
	STACK_TRACEBACK("(Depth) <= 0");\
	return eRet;\
}\

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

	//д�������ֵ
	template<typename T>
	static inline ErrCode WriteBigEndian(OutputStream &tData, const T &tVal) noexcept
	{
	MYTRY;
		ErrCode eRet = AllOk;
		if constexpr (sizeof(T) == 1)
		{
			tData.PutOnce((uint8_t)tVal);
		}
		else
		{
			//ͳһ���޷������ͣ���ֹ�з������ƴ���
			using UT = typename std::make_unsigned<T>::type;
			static_assert(sizeof(UT) == sizeof(T), "Unsigned type size mismatch");

			for (size_t i = sizeof(T); i > 0; --i)
			{
				tData.PutOnce((uint8_t)(((UT)tVal) >> (8 * (i - 1))));//������ȡ
			}
		}

		return eRet;
	MYCATCH;
	}

	static ErrCode PutName(OutputStream &tData, const NBT_Type::String &sName) noexcept
	{
	MYTRY;
		ErrCode eRet = AllOk;

		//��ȡstring����
		size_t szStringLength = sName.size();

		//����С�Ƿ��������
		if (szStringLength > (size_t)NBT_Type::StringLength_Max)
		{
			eRet = Error(StringTooLongError, tData, __FUNCSIG__ ": szStringLength[%zu] > StringLength_Max[%zu]",
				szStringLength, (size_t)NBT_Type::StringLength_Max);
			STACK_TRACEBACK("szStringLength Test");
			return eRet;
		}

		//������Ƴ���
		NBT_Type::StringLength wNameLength = (uint16_t)szStringLength;
		eRet = WriteBigEndian(tData, wNameLength);
		if (eRet < AllOk)
		{
			STACK_TRACEBACK("wNameLength Write");
			return eRet;
		}

		//�������
		tData.PutRange((const typename DataType::value_type *)sName.data(), sName.size());

		return eRet;
	MYCATCH;
	}

	template<typename T>
	static ErrCode PutbuiltInType(OutputStream &tData, const T &tBuiltIn) noexcept
	{
		ErrCode eRet = AllOk;

		//��ȡԭʼ���ͣ�Ȼ��ת����raw����׼��д��
		using RAW_DATA_T = NBT_Type::BuiltinRawType_T<T>;//ԭʼ����ӳ��
		RAW_DATA_T tTmpRawData = std::bit_cast<RAW_DATA_T>(tBuiltIn);

		eRet = WriteBigEndian(tData, tTmpRawData);
		if (eRet < AllOk)
		{
			STACK_TRACEBACK("tTmpRawData Write");
			return eRet;
		}

		return eRet;
	}

	template<typename T>
	static ErrCode PutArrayType(OutputStream &tData, const T &tArray) noexcept
	{
		ErrCode eRet = AllOk;

		//��ȡ�����С�ж��Ƿ񳬹�Ҫ������
		//Ҳ����4�ֽ��з�����������
		size_t szArrayLength = tArray.size();
		if (szArrayLength > (size_t)NBT_Type::ArrayLength_Max)
		{
			eRet = Error(ArrayTooLongError, tData, __FUNCSIG__ ": szArrayLength[%zu] > ArrayLength_Max[%zu]",
				szArrayLength, (size_t)NBT_Type::ArrayLength_Max);
			STACK_TRACEBACK("szArrayLength Test");
			return eRet;
		}

		//��ȡʵ��д����С
		NBT_Type::ArrayLength iArrayLength = (NBT_Type::ArrayLength)szArrayLength;
		eRet = WriteBigEndian(tData, iArrayLength);
		if (eRet < AllOk)
		{
			STACK_TRACEBACK("iArrayLength Write");
			return eRet;
		}

		//д��Ԫ��
		for (NBT_Type::ArrayLength i = 0; i < iArrayLength; ++i)
		{
			typename T::value_type tTmpData = tArray[i];
			eRet = WriteBigEndian(tData, tTmpData);
			if (eRet < AllOk)
			{
				STACK_TRACEBACK("tTmpData Write");
				return eRet;
			}
		}

		return eRet;
	}

	static ErrCode PutCompoundType(OutputStream &tData, const NBT_Type::Compound &tCompound, size_t szStackDepth) noexcept
	{
		ErrCode eRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);
		
		//ע��compound��Ϊ�������û��Ԫ���������ƵĽṹ
		//�˴��������С��������д����С
		for (const auto &[sName, nodeNbt] : tCompound)
		{
			NBT_TAG curTag = nodeNbt.GetTag();

			//�������������nbt end���͵�Ԫ�أ�ɾ���������
			if (curTag == NBT_TAG::End)
			{
				//EndԪ�ر����Ծ��棨���治���ش����룩
				Error(EndElementIgnoreWarn, tData, __FUNCSIG__ ": Name: \"%s\", type is [NBT_Type::End], ignored!",
					U16ANSI(U16STR(sName)).c_str());
				continue;
			}

			//��д��tag
			eRet = WriteBigEndian(tData, (NBT_TAG_RAW_TYPE)curTag);
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("curTag Write");
				return eRet;
			}

			//Ȼ��д��name
			eRet = PutName(tData, sName);
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("PutName Fail, Type: [NBT_Type::%s]", NBT_Type::GetTypeName(curTag));
				return eRet;
			}

			//������tag����д������
			eRet = PutSwitch(tData, nodeNbt, curTag, szStackDepth - 1);
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("PutSwitch Fail, Name: \"%s\", Type: [NBT_Type::%s]",
					U16ANSI(U16STR(sName)).c_str(), NBT_Type::GetTypeName(curTag));
				return eRet;
			}
		}

		//ע��Compound������һ��NBT_TAG::End��β�����д����������ǰ�淵�أ������ý�β
		eRet = WriteBigEndian(tData, (NBT_TAG_RAW_TYPE)NBT_TAG::End);
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("NBT_TAG::End[0x00(0)] Write");
			return eRet;
		}

		return eRet;
	}

	static ErrCode PutStringType(OutputStream &tData, const NBT_Type::String &tString) noexcept
	{
		ErrCode eRet = AllOk;

		eRet = PutName(tData, tString);//����PutNameʵ�֣���Ϊstring�ߵ�name��ͬ����
		if (eRet < AllOk)
		{
			STACK_TRACEBACK("PutString");//��Ϊ�ǽ���ʵ�֣���������СС�ĸĸ�������ֹ����Name����
			return eRet;
		}

		return eRet;
	}

	static ErrCode PutListType(OutputStream &tData, const NBT_Type::List &tList, size_t szStackDepth) noexcept
	{
		ErrCode eRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);

		//���
		size_t szListLength = tList.size();
		if (szListLength > (size_t)NBT_Type::ListLength_Max)//���ڵ������ǿ�Ƹ�ֵ�ᵼ���������⣬ֻ�ܷ��ش���
		{
			eRet = Error(ListTooLongError, tData, __FUNCSIG__ ": szListLength[%zu] > ListLength_Max[%zu]",
				szListLength, (size_t)NBT_Type::ListLength_Max);
			STACK_TRACEBACK("szListLength Test");
			return eRet;
		}

		//ת��Ϊд���С
		NBT_Type::ListLength iListLength = (NBT_Type::ListLength)szListLength;

		//�жϣ����Ȳ�Ϊ0����ӵ�пձ�ǩ
		NBT_TAG enListValueTag = tList.enElementTag;
		if (enListValueTag == NBT_TAG::End && iListLength != 0)
		{
			eRet = Error(ListElementTypeError, tData, __FUNCSIG__ ": The list with TAG_End[0x00] tag must be empty, but [%d] elements were found",
				iListLength);
			STACK_TRACEBACK("iListLength And enListValueTag Test");
			return eRet;
		}

		//��ȡ�б��ǩ������б���Ϊ0����ǿ�Ƹ�Ϊ�ձ�ǩ
		NBT_TAG enListElementTag = iListLength == 0 ? NBT_TAG::End : enListValueTag;

		//д����ǩ
		eRet = WriteBigEndian(tData, (NBT_TAG_RAW_TYPE)enListElementTag);
		if (eRet < AllOk)
		{
			STACK_TRACEBACK("enListElementTag Write");
			return eRet;
		}

		//д������
		eRet = WriteBigEndian(tData, iListLength);
		if (eRet < AllOk)
		{
			STACK_TRACEBACK("iListLength Write");
			return eRet;
		}

		//д���б��ݹ飩
		//д��ʱ�ж�Ԫ�ر�ǩ��enListElementTag��һ�µĴ���
		for (NBT_Type::ListLength i = 0; i < iListLength; ++i)
		{
			//��ȡԪ��������
			const NBT_Node &tmpNode = tList[i];
			NBT_TAG curTag = tmpNode.GetTag();

			//����ÿ��Ԫ�أ���������Ƿ����б�洢һ��
			if (curTag != enListElementTag)
			{
				eRet = Error(ListElementTypeError, tData, __FUNCSIG__ ": Expected type [NBT_Type::%s][0x%02X(%d)] in list, but found type [NBT_Type::%s][0x%02X(%d)]",
				NBT_Type::GetTypeName(enListElementTag), (NBT_TAG_RAW_TYPE)enListElementTag, (NBT_TAG_RAW_TYPE)enListElementTag,
				NBT_Type::GetTypeName(curTag), (NBT_TAG_RAW_TYPE)curTag, (NBT_TAG_RAW_TYPE)curTag);
				STACK_TRACEBACK("curTag Test");
				return eRet;
			}

			//һ�£��ܺã���ô���
			//�б������֣������ظ�tag��ֻ���������
			eRet = PutSwitch(tData, tmpNode, enListElementTag, szStackDepth - 1);
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("PutSwitch Error, Size: [%d] Index: [%d]", iListLength, i);
				return eRet;
			}
		}

		return eRet;
	}

	static ErrCode PutSwitch(OutputStream &tData, const NBT_Node &nodeNbt, NBT_TAG tagNbt, size_t szStackDepth) noexcept
	{
		ErrCode eRet = AllOk;

		switch (tagNbt)
		{
		case NBT_TAG::Byte:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Byte>;
				eRet = PutbuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::Short:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Short>;
				eRet = PutbuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::Int:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Int>;
				eRet = PutbuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::Long:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Long>;
				eRet = PutbuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::Float:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Float>;
				eRet = PutbuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::Double:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Double>;
				eRet = PutbuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::ByteArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::ByteArray>;
				eRet = PutArrayType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::String://����Ψһ����ģ�庯��
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::String>;
				eRet = PutStringType(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::List://���ܵݹ飬��Ҫ����szStackDepth
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::List>;
				eRet = PutListType(tData, nodeNbt.GetData<CurType>(), szStackDepth);
			}
			break;
		case NBT_TAG::Compound://���ܵݹ飬��Ҫ����szStackDepth
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Compound>;
				eRet = PutCompoundType(tData, nodeNbt.GetData<CurType>(), szStackDepth);
			}
			break;
		case NBT_TAG::IntArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::IntArray>;
				eRet = PutArrayType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::LongArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::LongArray>;
				eRet = PutArrayType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::End://ע��end��ǩ���Բ����Խ���
			{
				eRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": NBT Tag switch error: Unexpected Type Tag NBT_TAG::End[0x00(0)]");
			}
			break;
		default://���ݳ���
			{
				eRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": NBT Tag switch error: Unknown Type Tag[0x%02X(%d)]",
					(NBT_TAG_RAW_TYPE)tagNbt, (NBT_TAG_RAW_TYPE)tagNbt);//�˴���������ǰ���أ�����Ĭ�Ϸ��ش���
			}
			break;
		}

		if (eRet != AllOk)//���������һ��ջ����
		{
			STACK_TRACEBACK("Tag[0x%02X(%d)] write error!",
				(NBT_TAG_RAW_TYPE)tagNbt, (NBT_TAG_RAW_TYPE)tagNbt);
		}

		return eRet;
	}

public:
	//�����tData�У����ֹ��ܺ�ԭ�����ReadNBT����ע�ͣ�szDataStartIndex�ڴ˴����Զ�һ��tDataͨ����ͬ��tCompound��szDataStartIndex = tData.size()
	//�������Դﵽ�Ѷ����ͬ��nbt�����ͬһ��tData�ڵĹ���
	static bool WriteNBT(DataType &tData, const NBT_Type::Compound &tCompound, size_t szDataStartIndex = 0, size_t szStackDepth = 512) noexcept
	{
	MYTRY;
		//��ʼ������������
		OutputStream OptStream(tData, szDataStartIndex);

		//������ջ���
		//printf("Max Stack Depth [%zu]\n", szStackDepth);

		//��ʼ�ݹ����
		return PutCompoundType(OptStream, tCompound, szStackDepth) == AllOk;
	MYCATCH;//�Է���һ������Ҫ����һ��
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