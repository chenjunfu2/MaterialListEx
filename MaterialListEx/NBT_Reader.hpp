#pragma once

#include <new>//std::bad_alloc
#include <bit>//std::bit_cast
#include <string>
#include <stdint.h>
#include <stdlib.h>//byte swap
#include <type_traits>
#include <stdarg.h>//va��

#include "NBT_Node.hpp"

//�뾡����ʹ��basic_string_view�����ٿ���
//��Ϊstring��[]����������пӣ�ÿ�ζ����ж��Ƿ����Ż�ģʽ�������⣬���Դ洢dataָ��ֱ�ӷ����Լ���
//������������һ���
//cmp qword ptr [...],10h
//jb ...
//�����ж�string�Ƿ���С��16���Ż�ģʽ����������ô����string
template <typename T = std::basic_string_view<uint8_t>>
class MyInputStream
{
private:
	const T &tData = {};
	size_t szIndex = 0;
public:
	
	MyInputStream(const T &_tData, size_t szStartIdx = 0) :tData(_tData), szIndex(szStartIdx)
	{}
	~MyInputStream(void) = default;//Ĭ������

	MyInputStream(const MyInputStream &) = delete;
	MyInputStream(MyInputStream &&) = delete;

	MyInputStream &operator=(const MyInputStream &) = delete;
	MyInputStream &operator=(MyInputStream &&) = delete;

	const typename T::value_type &operator[](size_t szIndex) const noexcept
	{
		return tData[szIndex];
	}

	typename T::value_type GetNext() noexcept
	{
		return tData[szIndex++];
	}

	void GetRange(void *pDest, size_t szSize) noexcept
	{
		std::memcpy(pDest, &tData[szIndex], szSize);
		szIndex += szSize;
	}

	void UnGet() noexcept
	{
		if (szIndex != 0)
		{
			--szIndex;
		}
	}

	const typename T::value_type *CurData() const noexcept
	{
		return &(tData[szIndex]);
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

	const typename T::value_type *BaseData() const noexcept
	{
		return tData.data();
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

template <typename DataType = std::basic_string_view<uint8_t>>
class NBT_Reader//�뾡����ʹ��basic_string_view
{
	NBT_Reader(void) = delete;
	~NBT_Reader(void) = delete;

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

		//������ԣ�Ԥ��szCurrentǰ��n���ַ���������е��߽�
#define VIEW_PRE (4 * 8 + 3)//��ǰ
#define VIEW_SUF (4 * 8 + 5)//���
		size_t rangeBeg = (tData.Index() > VIEW_PRE) ? (tData.Index() - VIEW_PRE) : (0);//�ϱ߽����
		size_t rangeEnd = ((tData.Index() + VIEW_SUF) < tData.Size()) ? (tData.Index() + VIEW_SUF) : (tData.Size());//�±߽����
#undef VIEW_SUF
#undef VIEW_PRE
		//�����Ϣ
		printf
		(
			"Data Review:\n"\
			"Current: 0x%02llX(%zu)\n"\
			"Data Size: 0x%02llX(%zu)\n"\
			"Data[0x%02llX(%zu),0x%02llX(%zu)):\n",

			(uint64_t)tData.Index(), tData.Index(),
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

			if (i != tData.Index())
			{
				printf(" %02X ", (uint8_t)tData[i]);
			}
			else//����ǵ�ǰ�����ֽڣ��ӷ����ſ���
			{
				printf("[%02X]", (uint8_t)tData[i]);
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

	//��ȡ�������ֵ��bNoCheckΪtrue�򲻽����κμ��
	template<bool bNoCheck = false, typename T>
	requires std::integral<T>
	static inline std::conditional_t<bNoCheck, void, ErrCode> ReadBigEndian(InputStream &tData, T &tVal) noexcept
	{
		if constexpr (!bNoCheck)
		{
			if (!tData.HasAvailData(sizeof(T)))
			{
				ErrCode eRet = Error(OutOfRangeError, tData, "tData size [%zu], current index [%zu], remaining data size [%zu], but try to read [%zu]",
					tData.Size(), tData.Index(), tData.Size() - tData.Index(), sizeof(T));
				STACK_TRACEBACK("HasAvailData Test");
				return eRet;
			}
		}

		if constexpr (sizeof(T) == sizeof(uint8_t))
		{
			tVal = (T)(uint8_t)tData.GetNext();
		}
		else if constexpr (sizeof(T) == sizeof(uint16_t))
		{
			uint16_t tmp{};
			tData.GetRange((uint8_t *)&tmp, sizeof(tmp));
			tVal = (T)(uint16_t)_byteswap_ushort(tmp);
		}
		else if constexpr (sizeof(T) == sizeof(uint32_t))
		{
			uint32_t tmp{};
			tData.GetRange((uint8_t *)&tmp, sizeof(tmp));
			tVal = (T)(uint32_t)_byteswap_ulong(tmp);
		}
		else if constexpr (sizeof(T) == sizeof(uint64_t))
		{
			uint64_t tmp{};
			tData.GetRange((uint8_t *)&tmp, sizeof(tmp));
			tVal = (T)(uint64_t)_byteswap_uint64(tmp);
		}
		else//other
		{
			//ͳһ���޷�������
			using UT = typename std::make_unsigned<T>::type;
			static_assert(sizeof(UT) == sizeof(T), "Unsigned type size mismatch");

			//����������ȡ
			uint8_t u8BigEndianBuffer[sizeof(T)] = { 0 };
			tData.GetRange(u8BigEndianBuffer, sizeof(u8BigEndianBuffer));

			//��̬չ�������for��
			[&] <size_t... Is>(std::index_sequence<Is...>) -> void
			{
				UT tmpVal = 0;//��ʱ�����棬���������һֱ�������÷�����ɿ���
				((tmpVal |= ((UT)u8BigEndianBuffer[Is]) << (8 * (sizeof(T) - Is - 1))), ...);//ģ��չ��λ����

				//��ȡ��ɸ�ֵ��tVal
				tVal = (T)tmpVal;
			}(std::make_index_sequence<sizeof(T)>{});

			//����������ȼ۵����ܽϵ͵�д�����������������ӿɶ���
			//for (size_t i = sizeof(T); i > 0; --i)
			//{
			//	tVal |= ((UT)(uint8_t)tData.GetNext()) << (8 * (i - 1));//ÿ���ƶ��ղ���ȡ�ĵ�λ����λ��Ȼ�������ȡ
			//}
		}

		if constexpr (!bNoCheck)
		{
			return AllOk;
		}
	}

	static ErrCode GetName(InputStream &tData, NBT_Type::String &tName) noexcept
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
			STACK_TRACEBACK("HasAvailData Test");
			return eRet;
		}

		
		//����������
		tName.reserve(wStringLength);//��ǰ����
		tName.assign((const NBT_Type::String::value_type *)tData.CurData(), wStringLength);//����string���������Ϊ0����0���ַ������Ϸ���Ϊ��
		tData.AddIndex(wStringLength);//�ƶ��±�

		return eRet;
	MYCATCH;
	}

	template<typename T>
	static ErrCode GetBuiltInType(InputStream &tData, T &tBuiltIn) noexcept
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
		tBuiltIn = std::move(std::bit_cast<T>(tTmpRawData));
		return eRet;
	}

	template<typename T>
	static ErrCode GetArrayType(InputStream &tData, T &tArray) noexcept
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
		tArray.reserve(iElementCount);//��ǰ����
		//��ȡdElementCount��Ԫ��
		for (NBT_Type::ArrayLength i = 0; i < iElementCount; ++i)
		{
			typename T::value_type tTmpData{};
			ReadBigEndian<true>(tData, tTmpData);//������Ҫȷ����Χ��ȫ
			tArray.emplace_back(std::move(tTmpData));//��ȡһ������һ��
		}

		return eRet;
	MYCATCH;
	}

	//����ǷǸ������ж�����
	template<bool bRoot>
	static ErrCode GetCompoundType(InputStream &tData, NBT_Type::Compound &tCompound, size_t szStackDepth) noexcept
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
				eRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": NBT Tag switch default: Unknown Type Tag[0x%02X(%d)]",
					(NBT_TAG_RAW_TYPE)tagNbt, (NBT_TAG_RAW_TYPE)tagNbt);//�˴���������ǰ���أ�����Ĭ�Ϸ��ش���
				STACK_TRACEBACK("tagNbt Test");
				return eRet;//������Χ���̷���
			}

			//Ȼ���ȡ����
			NBT_Type::String sName{};
			eRet = GetName(tData, sName);
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("GetName Fail, Type: [NBT_Type::%s]", NBT_Type::GetTypeName(tagNbt));
				return eRet;//���ƶ�ȡʧ�����̷���
			}

			//Ȼ��������ͣ����ö�Ӧ�����Ͷ�ȡ�����ص�tmpNode
			NBT_Node tmpNode{};
			eRet = GetSwitch(tData, tmpNode, tagNbt, szStackDepth - 1);
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("GetSwitch Fail, Name: \"%s\", Type: [NBT_Type::%s]", sName.ToCharTypeUTF8().c_str(), NBT_Type::GetTypeName(tagNbt));
				//return eRet;//ע��˴������أ����в��룬�Ա��������֮ǰ����ȷ����
			}

			//sName:tmpNode�����뵱ǰ����ջ��ȵĸ��ڵ�
			//����ʵ��mc java����ó����������һ���Ѿ����ڵļ����ᵼ��ԭ�ȵ�ֵ���滻������
			//��ô��ʧ�ܺ��ֶ��ӵ������滻��ǰֵ��ע�⣬�˴�������try_emplace����Ϊtry_emplaceʧ�ܺ�ԭ�ȵ�ֵ
			//tmpNode���ᱻ�ƶ����¶�ʧ������Ҳ���追�������Է�ֹ�ƶ���ʧ����
			auto [it,bSuccess] = tCompound.try_emplace(std::move(sName), std::move(tmpNode));
			if (!bSuccess)
			{
				//ʹ�õ�ǰֵ�滻����ֹ�����ԭʼֵ
				it->second = std::move(tmpNode);

				//�������棬ע�⾯�治��eRet�ӷ���ֵ
				Error(ElementExistsWarn, tData, __FUNCSIG__ ": Name: \"%s\", Type: [NBT_Type::%s] data already exist!",
					sName.ToCharTypeUTF8().c_str(), NBT_Type::GetTypeName(tagNbt));
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

	static ErrCode GetStringType(InputStream &tData, NBT_Type::String &tString) noexcept
	{
		ErrCode eRet = AllOk;

		//��ȡ�ַ���
		eRet = GetName(tData, tString);//��Ϊstring��name��ȡԭ��һ�£�ֱ�ӽ���ʵ��
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("GetString");//��Ϊ�ǽ���ʵ�֣���������СС�ĸĸ�������ֹ����Name����
			return eRet;
		}

		return eRet;
	}

	static ErrCode GetListType(InputStream &tData, NBT_Type::List &tList, size_t szStackDepth) noexcept
	{
	MYTRY;
		ErrCode eRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);

		//��ȡ1�ֽڵ��б�Ԫ������
		NBT_TAG_RAW_TYPE enListElementTag = 0;//b=byte
		eRet = ReadBigEndian(tData, enListElementTag);
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("enListElementTag Read");
			return eRet;
		}

		//������б�Ԫ������
		if (enListElementTag >= NBT_TAG::ENUM_END)
		{
			eRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": List NBT Type:Unknown Type Tag[0x%02X(%d)]",
				(NBT_TAG_RAW_TYPE)enListElementTag, (NBT_TAG_RAW_TYPE)enListElementTag);
			STACK_TRACEBACK("enListElementTag Test");
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
		if (enListElementTag == NBT_TAG::End && iListLength != 0)
		{
			eRet = Error(ListElementTypeError, tData, __FUNCSIG__ ": The list with TAG_End[0x00] tag must be empty, but [%d] elements were found",
				iListLength);
			STACK_TRACEBACK("enListElementTag And iListLength Test");
			return eRet;
		}

		//ȷ���������Ϊ0������£��б����ͱ�ΪEnd
		if (iListLength == 0 && enListElementTag != NBT_TAG::End)
		{
			enListElementTag = (NBT_TAG_RAW_TYPE)NBT_TAG::End;
		}

		//�������Ͳ���ǰ����
		tList.enElementTag = (NBT_TAG)enListElementTag;//����������
		tList.reserve(iListLength);//��֪��С��ǰ������ٿ���

		//����Ԫ�����ͣ���ȡn���б�
		for (NBT_Type::ListLength i = 0; i < iListLength; ++i)
		{
			NBT_Node tmpNode{};//�б�Ԫ�ػ�ֱ�Ӹ�ֵ�޸�
			eRet = GetSwitch(tData, tmpNode, (NBT_TAG)enListElementTag, szStackDepth - 1);
			if (eRet != AllOk)//������
			{
				STACK_TRACEBACK("GetSwitch Error, Size: [%d] Index: [%d]", iListLength, i);
				return eRet;
			}

			//ÿ��ȡһ���������һ��
			tList.emplace_back(std::move(tmpNode));
		}
		
		return eRet;
	MYCATCH;
	}

	//����������������ڲ����ò������쳣�������أ����Դ˺������Բ��׳��쳣���ɴ˵��ô˺����ĺ���Ҳ������catch�쳣
	static ErrCode GetSwitch(InputStream &tData, NBT_Node &nodeNbt, NBT_TAG tagNbt, size_t szStackDepth) noexcept//ѡ���������ݹ�㣬�ɺ������õĺ������
	{
		ErrCode eRet = AllOk;

		switch (tagNbt)
		{
		case NBT_TAG::Byte:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Byte>;
				nodeNbt.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::Short:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Short>;
				nodeNbt.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::Int:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Int>;
				nodeNbt.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::Long:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Long>;
				nodeNbt.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::Float:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Float>;
				nodeNbt.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::Double:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Double>;
				nodeNbt.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::ByteArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::ByteArray>;
				nodeNbt.emplace<CurType>();
				eRet = GetArrayType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::String:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::String>;
				nodeNbt.emplace<CurType>();
				eRet = GetStringType(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::List://��Ҫ�ݹ���ã��б�ͷ������ǩID�ͳ��ȣ�������Ϊһϵ��ͬ���ͱ�ǩ����Ч���أ��ޱ�ǩ ID �����ƣ�
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::List>;
				nodeNbt.emplace<CurType>();
				eRet = GetListType(tData, nodeNbt.GetData<CurType>(), szStackDepth);//ѡ���������ٵݹ��
			}
			break;
		case NBT_TAG::Compound://��Ҫ�ݹ����
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Compound>;
				nodeNbt.emplace<CurType>();
				eRet = GetCompoundType<false>(tData, nodeNbt.GetData<CurType>(), szStackDepth);//ѡ���������ٵݹ��
			}
			break;
		case NBT_TAG::IntArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::IntArray>;
				nodeNbt.emplace<CurType>();
				eRet = GetArrayType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::LongArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::LongArray>;
				nodeNbt.emplace<CurType>();
				eRet = GetArrayType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::End://��Ӧ�����κ�ʱ�������˱�ǩ��Compound���ȡ�������ĵ������ᴫ�룬List�����˱�ǩ������ö�ȡ������������Ϊ����
			{
				eRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": NBT Tag switch error: Unexpected Type Tag NBT_TAG::End[0x00(0)]");
			}
			break;
		default://����δ֪��ǩ����NBT�ڱ�����ǩ����
			{
				eRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": NBT Tag switch error: Unknown Type Tag[0x%02X(%d)]",
					(NBT_TAG_RAW_TYPE)tagNbt, (NBT_TAG_RAW_TYPE)tagNbt);//�˴���������ǰ���أ�����Ĭ�Ϸ��ش���
			}
			break;
		}
		
		if (eRet != AllOk)//���������һ��ջ����
		{
			STACK_TRACEBACK("Tag[0x%02X(%d)] read error!",
				(NBT_TAG_RAW_TYPE)tagNbt, (NBT_TAG_RAW_TYPE)tagNbt);
		}

		return eRet;//���ݷ���ֵ
	}

public:
	/*
	��ע���˺�����ȡnbtʱ���ᴴ��һ��Ĭ�ϸ���Ȼ���nbt���������ݼ��ϵ���Ĭ�ϸ��ϣ�
	Ҳ�������°���mojang��nbt��׼��Ĭ�ϸ�������Compound��Ҳ�ᱻ�ҽӵ�����ֵ���
	NBT_Type::Compound�С������������ص�NBT_Type::Compound���ɵõ�����NBT���ݣ�
	��ô����Ŀ����Ϊ�˷����д�����Ҳ�����ĳЩ�ط�����ʵ��mojang������compound��
	���⴦����������¿�����һ���̶���������mojang��׼֧�ָ����NBT�ļ������
	�����ļ��ڲ�����Compound��ʼ�ģ����ǵ����ļ�����ͬ�����Ҵ������ֵ�NBT����ôҲ��
	������ȡ����ȫ������NBT_Type::Compound�У��ͺ���nbt�ļ��������һ���������
	NBT_Type::Compoundһ������Եģ�д������Ҳ��֧��д���������������д������
	WriteNBT��д����ʱ�򣬴����ֵҲ��һ���ں�NBT_Type::Compound��NBT_Node��
	Ȼ�����NBT_Type::Compound�����ᱻ���κ���ʽд��NBT�ļ��������ڲ����ݣ�
	Ҳ���ǹҽ�����������ݻᱻд�룬�������ܱ�֤����mojang��nbt�ļ���Ҳ��һ���̶���
	��չnbt�ļ��ڿ��Դ洢�����ݣ�����nbt�ļ�ֱ�Ӵ洢�����ֵ�Զ����Ǳ����ȹ���һ��
	�����Ƶ�Compound�£�
	*/

	//szStackDepth ����ջ��ȣ��ݹ������ɿ�Ƕ�׵Ŀ��ܽ��еݹ�ĺ������У�ջ��ȵݼ����ɶ�ѡ�����ĵ��ý���
	//ע��˺����������tCompound�����Կ��Զ�һ��tCompoundͨ����ͬ��tData��ε�������ȡ���nbtƬ�β��ϲ���һ��
	//���ָ����szDataStartIndex������tData�г���ΪszDataStartIndex������
	static bool ReadNBT(NBT_Type::Compound &tCompound, const DataType &tData, size_t szDataStartIndex = 0, size_t szStackDepth = 512) noexcept//��data�ж�ȡnbt
	{
	MYTRY;
		//��ʼ������������
		InputStream IptStream(tData, szDataStartIndex);

		//������ջ���
		//printf("Max Stack Depth [%zu]\n", szStackDepth);

		//��ʼ�ݹ��ȡ
		return GetCompoundType<true>(IptStream, tCompound, szStackDepth) == AllOk;//��data�л�ȡnbt���ݵ�nRoot�У�ֻ�д˵���Ϊ�������ã�ģ��true�������ڴ����������
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