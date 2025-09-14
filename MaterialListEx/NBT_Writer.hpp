#pragma once

#include <new>//bad alloc
#include <string>
#include <stdint.h>
#include <type_traits>

#include "NBT_Node.hpp"

template <typename T>
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

	template<typename V>
	requires(std::is_constructible_v<T::value_type, V &&>)
	void PutOnce(V &&c)
	{
		tData.push_back(std::forward<V>(c));
	}

	void PutRange(typename T::const_iterator itBeg, typename T::const_iterator itEnd)
	{
		tData.append(itBeg, itEnd);
	}

	void UnPut(void) noexcept
	{
		tData.pop_back();
	}

	size_t GetSize(void) const noexcept
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
	using OutputStream = MyOutputStream<DataType>;//������
private:
	enum ErrCode : int
	{
		ERRCODE_END = -9,//������ǣ�ͳ�Ƹ������ִ�С

		UnknownError,//��������
		StdException,//��׼�쳣
		OutOfMemoryError,//�ڴ治�����
		ListElementTypeError,//�б�Ԫ�����ʹ��󣨴������⣩
		StackDepthExceeded,//����ջ��ȹ���������⣩
		StringTooLongError,//�ַ�����������
		ArrayTooLongError,//�����������
		ListTooLongError,//�б��������

		AllOk,//û������
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
		"StringTooLongError",
		"ArrayTooLongError",
		"ListTooLongError",

		"AllOk",
	};

	//�ǵ�ͬ�����飡
	static_assert(sizeof(errReason) / sizeof(errReason[0]) == (-ERRCODE_END), "errReason array out sync");


	enum WarnCode : int
	{
		NoWarn = 0,

		WARNCODE_END,
	};

	//ȷ��[�Ǿ�����]Ϊ�㣬��ֹ���ַǷ���[�Ǿ�����]�����ж�ʧЧ�������
	static_assert(NoWarn == 0, "NoWarn != 0");

	static inline const char *const warnReason[] =//�������飬ֱ����WarnCode����
	{
		"NoWarn",
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
	static int _cdecl Error(const T &code, const OutputStream &tData, _Printf_format_string_ const char *const cpExtraInfo = NULL, ...)
	{
		/*�������ջ���ݣ������ϸ��������nbtǶ��·��*/



		return code;
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

	//��С��ת��
	template<typename T>
	static inline int WriteBigEndian(OutputStream &tData, const T &tVal)
	{
		int iRet = AllOk;
		if constexpr (sizeof(T) == 1)
		{
			MYTRY
			tData.PutOnce((uint8_t)tVal);
			MYCATCH_BADALLOC
		}
		else
		{
			//ͳһ���޷������ͣ���ֹ�з������ƴ���
			using UT = typename std::make_unsigned<T>::type;
			UT tTmp = (UT)tVal;
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				MYTRY
				tData.PutOnce((uint8_t)tTmp);
				MYCATCH_BADALLOC
				tTmp >>= 8;
			}
		}

		return iRet;
	}

	//PutName
	static int PutName(OutputStream &tData, const NBT_Type::String &sName)
	{
		int iRet = AllOk;

		//��ȡstring����
		size_t szStringLength = sName.size();

		//����С�Ƿ��������
		if (szStringLength > (size_t)NBT_Type::StringLength_Max)
		{
			iRet = Error(StringTooLongError, tData, __FUNCSIG__ ": sName.length()[%zu] > UINT16_MAX[%zu]", sName.length(), UINT16_MAX);
			//STACK_TRACEBACK
			return iRet;
		}

		//������Ƴ���
		NBT_Type::StringLength wNameLength = (uint16_t)szStringLength;
		iRet = WriteBigEndian(tData, wNameLength);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("wNameLength Write");
			return iRet;
		}

		//�������
		MYTRY
		tData.PutRange(sName.begin(), sName.end());
		MYCATCH_BADALLOC

		return AllOk;
	}

	template<typename T>
	static int PutbuiltInType(OutputStream &tData, const NBT_Node &nRoot)
	{
		int iRet = AllOk;

		//��ȡԭʼ���ͣ�Ȼ��ת����raw����׼��д��
		const T &tBuiltIn = nRoot.GetData<T>();

		using RAW_DATA_T = NBT_Type::BuiltinRawType_T<T>;//ԭʼ����ӳ��
		RAW_DATA_T tTmpRawData = std::bit_cast<RAW_DATA_T>(tBuiltIn);

		iRet = WriteBigEndian(tData, tTmpRawData);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("Name: \"%s\" tTmpRawData Write");
			return iRet;
		}

		return iRet;
	}

	template<typename T>
	static int PutArrayType(OutputStream &tData, const NBT_Node &nRoot)
	{
		int iRet = AllOk;

		const T &tArray = nRoot.GetData<T>();

		//��ȡ�����С�ж��Ƿ񳬹�Ҫ������
		//Ҳ����4�ֽ��з�����������
		size_t szArrayLength = tArray.size();
		if (szArrayLength > (size_t)NBT_Type::ArrayLength_Max)
		{
			//error
			//stack
			return iRet;
		}

		//��ȡʵ��д����С
		NBT_Type::ArrayLength iArrayLength = (NBT_Type::ArrayLength)szArrayLength;
		iRet = WriteBigEndian(tData, iArrayLength);
		if (iRet < AllOk)
		{
			//stack
			return iRet;
		}

		//д��Ԫ��
		for (NBT_Type::ArrayLength i = 0; i < iArrayLength; ++i)
		{
			typename T::value_type tTmpData = tArray[i];
			iRet = WriteBigEndian(tData, tTmpData);
			if (iRet < AllOk)
			{
				//stack
				return iRet;
			}
		}

		return iRet;
	}

	static int PutCompoundType(OutputStream &tData, const NBT_Node &nRoot, size_t szStackDepth)
	{
		int iRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);
		//�������������nbt end���͵�Ԫ�أ�ɾ���������



		return iRet;
	}

	static int PutStringType(OutputStream &tData, const NBT_Node &nRoot)
	{
		int iRet = AllOk;

		const NBT_Type::String &tString = nRoot.GetData<NBT_Type::String>();
		iRet = PutName(tData, tString);//����PutNameʵ�֣���Ϊstring�ߵ�name��ͬ����
		if (iRet < AllOk)
		{
			//stack
			return iRet;
		}


		return iRet;
	}

	static int PutListType(OutputStream &tData, const NBT_Node &nRoot, size_t szStackDepth)
	{
		int iRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);

		const NBT_Type::List tList = nRoot.GetData<NBT_Type::List>();

		//���
		size_t szListLength = tList.size();
		if (szListLength > (size_t)NBT_Type::ListLength_Max)//���ڵ������ǿ�Ƹ�ֵ�ᵼ���������⣬ֻ�ܷ��ش���
		{
			//error
			//stack
			return iRet;
		}

		//ת��Ϊд���С
		NBT_Type::ListLength iListLength = (NBT_Type::ListLength)szListLength;

		//�жϣ����Ȳ�Ϊ0����ӵ�пձ�ǩ
		NBT_TAG enListValueTag = tList.enElementTag;
		if (iListLength != 0 && enListValueTag == NBT_TAG::End)
		{
			//error
			//stack
			return iRet;
		}

		//��ȡ�б��ǩ������б���Ϊ0����ǿ�Ƹ�Ϊ�ձ�ǩ
		NBT_TAG bListElementType = iListLength == 0 ? NBT_TAG::End : enListValueTag;

		//д����ǩ
		iRet = WriteBigEndian(tData, (NBT_TAG_RAW_TYPE)bListElementType);
		if (iRet < AllOk)
		{
			//stack
			return iRet;
		}

		//д������
		iRet = WriteBigEndian(tData, iListLength);
		if (iRet < AllOk)
		{
			//stack
			return iRet;
		}

		//д���б��ݹ飩
		//д��ʱ�ж�Ԫ�ر�ǩ��bListElementType��һ�µĴ���
		for (NBT_Type::ListLength i = 0; i < iListLength; ++i)
		{
			//������ʱ��ʵ�֣���Ҫ��������SwitchNBT�ݹ�


		}

		return iRet;
	}

	//static int SwitchNBT(OutputStream &tData, const NBT_Node &nRoot)

	//static int PutNBT(OutputStream &tData, const NBT_Node &nRoot)

public:
	NBT_Writer(void) = delete;
	~NBT_Writer(void) = delete;

	static bool WriteNBT(DataType &tData, const NBT_Node &nRoot)
	{


		return true;
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