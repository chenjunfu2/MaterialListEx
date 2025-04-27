#pragma once

#include "NBT_Node.hpp"
#include <new>//bad alloc

template <typename T>
class MyOutputStream
{
	template<typename T>
	struct has_emplace_back
	{
	private:
		template<typename U>
		static auto test(int) -> decltype(std::declval<U>().emplace_back(std::declval<typename U::value_type>()), std::true_type{});

		template<typename>
		static std::false_type test(...);

	public:
		static constexpr bool value = std::true_type::value;
	};
private:
	T &tData;
public:
	MyOutputStream(T &_tData, size_t szStartIdx = 0) :tData(_tData)
	{
		tData.resize(szStartIdx);
	}
	~MyOutputStream() = default;

	bool PutOnce(const typename T::value_type &c) noexcept
	{
		try
		{
			tData.push_back(c);
			return true;
		}
		catch (const std::bad_alloc&)
		{
			return false;
		}
	}

	bool PutOnce(typename T::value_type &&c) noexcept
	{
		try
		{
			tData.push_back(std::move(c));
			return true;
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
	}

	bool PutRange(typename T::const_iterator itBeg, typename T::const_iterator itEnd) noexcept
	{
		try
		{
			tData.append(itBeg, itEnd);
			return true;
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
	}

	template<typename... Args, typename = std::enable_if_t<has_emplace_back<T>::value>>
	bool EmplaceOnce(Args&&... args) noexcept
	{
		try
		{
			tData.emplace_back(std::forward<Args>(args)...);
			return true;
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
	}

	void UnPut() noexcept
	{
		tData.pop_back();
	}

	size_t GetSize() const noexcept
	{
		return tData.size();
	}

	void Reset() noexcept
	{
		tData.clear();
	}

	const T &Data() const noexcept
	{
		return tData;
	}

	T &Data() noexcept
	{
		return tData;
	}
};


template <typename DataType = std::string>
class NBT_Writer
{
	using OutputStream = MyOutputStream<DataType>;//������
private:
	enum ErrCode : int
	{
		ERRCODE_END = -2,

		OutOfMemoryError,//�ڴ治�����
		StringTooLongError,
		AllOk,
	};

	//ȷ��[�Ǵ�����]Ϊ�㣬��ֹ���ַǷ���[�Ǵ�����]�����ж�ʧЧ�������
	static_assert(AllOk == 0, "AllOk != 0");

	static inline const char *const errReason[] =//�����������㷽ʽ��(-ERRCODE_END - 1) + ErrCode
	{
		"StringTooLongError",
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
	template <typename T, typename std::enable_if<std::is_same<T, ErrCode>::value || std::is_same<T, WarnCode>::value, int>::type = 0>
	static int _cdecl Error(const T &code, const OutputStream &tData, _Printf_format_string_ const char *const cpExtraInfo = NULL, ...)
	{
		/*�������ջ���ݣ������ϸ��������nbtǶ��·��*/



		return code;
	}

	//��С��ת��
	template<typename T>
	static inline int WriteBigEndian(OutputStream &tData, const T &tVal)
	{
		int iRet = AllOk;
		if constexpr (sizeof(T) == 1)
		{
			if (!tData.PutOnce((uint8_t)tVal))
			{
				iRet = OutOfMemoryError;
				//iRet = Error(xxx);
				//STACK_TRACEBACK
				//return iRet;
				return iRet;
			}
		}
		else
		{
			//ͳһ���޷������ͣ���ֹ�з������ƴ���
			using UT = typename std::make_unsigned<T>::type;
			UT tTmp = (UT)tVal;
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				if (!tData.PutOnce((uint8_t)tTmp))
				{	
					iRet = OutOfMemoryError;
					//iRet = Error(xxx);
					//STACK_TRACEBACK
					//return iRet;
					return iRet;
				}
				tTmp >>= 8;
			}
		}

		return iRet;
	}

	//PutName
	static int PutName(OutputStream &tData, const NBT_Node::NBT_String &sName)
	{
		int iRet = AllOk;
		//����С�Ƿ��������
		if (sName.length() > UINT16_MAX)
		{
			iRet = Error(StringTooLongError, tData, __FUNCSIG__ ": sName.length()[%zu] > UINT16_MAX[%zu]", sName.length(), UINT16_MAX);
			//STACK_TRACEBACK
			return iRet;
		}

		//������Ƴ���
		uint16_t wNameLength = (uint16_t)sName.length();
		iRet = WriteBigEndian(tData, wNameLength);
		if (iRet < AllOk)
		{

			

		}
		//�������
		if (!tData.PutRange(sName.begin(), sName.end()))
		{


		}

		return AllOk;
	}

	//PutbuiltInType

	//PutArrayType

	//PutCompoundType

	//PutStringType

	//PutListType

	//SwitchNBT

	//PutNBT

public:
	NBT_Writer(void) = delete;
	~NBT_Writer(void) = delete;

	static bool WriteNBT(DataType &tData, const NBT_Node &nRoot)
	{

	}
};