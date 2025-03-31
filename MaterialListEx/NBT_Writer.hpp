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
		return tData.push_back(c);
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


template <typename DataType = std::string, typename OutputStream = MyOutputStream<DataType>>//Á÷ÀàÐÍ
class NBT_Writer
{
private:



public:
	NBT_Writer(void) = delete;
	~NBT_Writer(void) = delete;

	static bool WriteNBT(std::string &tData, const NBT_Node &nRoot)
	{

	}
};