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
	};

	//�ǵ�ͬ�����飡
	static_assert(sizeof(errReason) / sizeof(errReason[0]) == (-ERRCODE_END), "errReason array out sync");

	enum WarnCode : uint8_t
	{
		NoWarn = 0,

		WARNCODE_END,
	};

	constexpr static inline const char *const warnReason[] =//�������飬ֱ����WarnCode����
	{
		"NoWarn",
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

	//��С��ת��
	template<typename T>
	static ErrCode WriteBigEndian(OutputStream &tData, const T &tVal) noexcept
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
			UT tTmp = (UT)tVal;
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				tData.PutOnce((uint8_t)tTmp);
				tTmp >>= 8;
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
			eRet = Error(StringTooLongError, tData, __FUNCSIG__ ": sName.length()[%zu] > UINT16_MAX[%zu]", sName.length(), UINT16_MAX);
			//STACK_TRACEBACK
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
		tData.PutRange(sName.begin(), sName.end());

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
			STACK_TRACEBACK("Name: \"%s\" tTmpRawData Write");
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
			//error
			//stack
			return eRet;
		}

		//��ȡʵ��д����С
		NBT_Type::ArrayLength iArrayLength = (NBT_Type::ArrayLength)szArrayLength;
		eRet = WriteBigEndian(tData, iArrayLength);
		if (eRet < AllOk)
		{
			//stack
			return eRet;
		}

		//д��Ԫ��
		for (NBT_Type::ArrayLength i = 0; i < iArrayLength; ++i)
		{
			typename T::value_type tTmpData = tArray[i];
			eRet = WriteBigEndian(tData, tTmpData);
			if (eRet < AllOk)
			{
				//stack
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
		for (const auto &it : tCompound)
		{
			NBT_TAG curTag = it.second.GetTag();

			//�������������nbt end���͵�Ԫ�أ�ɾ���������
			if (curTag == NBT_TAG::End)
			{
				continue;
			}

			//��д��tag
			eRet = WriteBigEndian((NBT_TAG_RAW_TYPE)curTag);
			if (eRet != AllOk)
			{
				//stack
				break;
			}

			//Ȼ��д��name
			eRet = PutName(tData, it.first);
			if (eRet != AllOk)
			{
				//stack
				break;
			}

			//������tag����д������
			eRet = PutSwitch(tData, it.second, curTag, szStackDepth - 1);
			if (eRet != AllOk)
			{
				//stack
				break;
			}
		}

		return eRet;
	}

	static ErrCode PutStringType(OutputStream &tData, const NBT_Type::String &tString) noexcept
	{
		ErrCode eRet = AllOk;

		eRet = PutName(tData, tString);//����PutNameʵ�֣���Ϊstring�ߵ�name��ͬ����
		if (eRet < AllOk)
		{
			//stack
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
			//error
			//stack
			return eRet;
		}

		//ת��Ϊд���С
		NBT_Type::ListLength iListLength = (NBT_Type::ListLength)szListLength;

		//�жϣ����Ȳ�Ϊ0����ӵ�пձ�ǩ
		NBT_TAG enListValueTag = tList.enElementTag;
		if (iListLength != 0 && enListValueTag == NBT_TAG::End)
		{
			//error
			//stack
			return eRet;
		}

		//��ȡ�б��ǩ������б���Ϊ0����ǿ�Ƹ�Ϊ�ձ�ǩ
		NBT_TAG enListElementTag = iListLength == 0 ? NBT_TAG::End : enListValueTag;

		//д����ǩ
		eRet = WriteBigEndian(tData, (NBT_TAG_RAW_TYPE)enListElementTag);
		if (eRet < AllOk)
		{
			//stack
			return eRet;
		}

		//д������
		eRet = WriteBigEndian(tData, iListLength);
		if (eRet < AllOk)
		{
			//stack
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
				//error
				//stack
				return eRet;
			}

			//һ�£��ܺã���ô���
			//�б������֣������ظ�tag��ֻ���������
			eRet = PutSwitch(tData, tmpNode, enListElementTag, szStackDepth - 1);
			if (eRet != AllOk)
			{
				//stack
				return eRet;
			}
		}

		return eRet;
	}

	static ErrCode PutSwitch(OutputStream &tData, const NBT_Node &nRoot, NBT_TAG tagNbt, size_t szStackDepth) noexcept
	{
		ErrCode eRet = AllOk;





		return eRet;
	}

public:
	NBT_Writer(void) = delete;
	~NBT_Writer(void) = delete;

	static bool WriteNBT(DataType &tData, const NBT_Node &nRoot, size_t szStackDepth = 512)
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