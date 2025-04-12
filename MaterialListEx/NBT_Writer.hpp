#pragma once

#include "NBT_Node.hpp"

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

	void PutOnce(const typename T::value_type &c)
	{
		tData.push_back(c);
	}

	void PutOnce(typename T::value_type &&c)
	{
		tData.push_back(std::move(c));
	}

	void PutRange(typename T::iterator itBeg, typename T::iterator itEnd)
	{
		tData.append(itBeg, itEnd);
	}

	template<typename... Args, typename = std::enable_if_t<has_emplace_back<T>::value>>
	void EmplaceOnce(Args&&... args)
	{
		tData.emplace_back(std::forward<Args>(args)...);
	}

	void UnPut()
	{
		tData.pop_back();
	}

	size_t GetSize() const
	{
		return tData.size();
	}

	void Reset()
	{
		tData.resize(0);
	}

	const T &Data() const
	{
		return tData;
	}

	T &Data()
	{
		return tData;
	}
};


template <typename DataType = std::string>
class NBT_Writer
{
	using OutputStream = MyOutputStream<DataType>;//流类型
private:
	//大小端转换
	template<typename T>
	static inline void __WriteBigEndian(OutputStream &tData, const T &tVal)
	{
		if constexpr (sizeof(T) == 1)
		{
			tData.PutOnce((uint8_t)tVal);
		}
		else
		{
			//统一到无符号类型，防止有符号右移错误
			using UT = typename std::make_unsigned<T>::type;
			UT tTmp = (UT)tVal;
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				tData.PutOnce((uint8_t)tTmp);
				tTmp >>= 8;
			}
		}
	}

	template<bool bNoExcept = false, typename T>
	static inline std::conditional_t<bNoExcept, bool, void> WriteBigEndian(OutputStream &tData, const T &tVal) noexcept(bNoExcept)
	{
		if constexpr (bNoExcept)
		{
			try
			{
				__WriteBigEndian<T>(tData, tVal);
				return true;
			}
			catch (...)
			{
				return false;
			}
		}
		else
		{
			__WriteBigEndian(tData, tVal);
		}
	}

	//PutName

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