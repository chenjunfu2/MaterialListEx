#pragma once

#include <vector>
#include <compare>
#include <type_traits>

#include "NBT_TAG.hpp"

template<typename List>
class MyList :public List
{
private:
	//列表元素类型（只能一种元素）
	NBT_TAG tagValue = NBT_TAG::TAG_End;
	//此变量仅用于规约列表元素类型，无需参与比较与hash，但是需要参与构造与移动
private:
	bool TestAndSetType(NBT_TAG targetTag)
	{
		if (tagValue == NBT_TAG::TAG_End)
		{
			tagValue = targetTag;
			return true;
		}

		return targetTag == tagValue;
	}
public:
	//继承基类构造
	using List::List;

	MyList(void) = default;
	MyList(NBT_TAG _tagValType) :List({}), tagValue(_tagValType)
	{}
	MyList(MyList &&_Move) noexcept
		:List(std::move(_Move)),
		 tagValue(std::move(_Move.tagValue))
	{
		_Move.tagValue = NBT_TAG::TAG_End;
	}
	MyList(const MyList &_Other)
		:List(_Other),
		tagValue(_Other.tagValue)
	{}

	MyList &operator=(MyList &&_Move) noexcept
	{
		List::operator=(std::move(_Move));
		tagValue = std::move(_Move.tagValue);

		_Move.tagValue = NBT_TAG::TAG_End;

		return *this;
	}

	MyList &operator=(const MyList &_Other)
	{
		List::operator=(_Other);
		tagValue = _Other.tagValue;

		return *this;
	}

	bool SetTag(NBT_TAG tagNewValue)
	{
		if (!List::empty())
		{
			return false;
		}

		tagValue = tagNewValue;
		return true;
	}

	NBT_TAG GetTag(void)
	{
		return tagValue;
	}

	//简化list查询
	inline typename List::value_type &Get(const typename List::size_type &szPos)
	{
		return List::at(szPos);
	}

	inline const typename List::value_type &Get(const typename List::size_type &szPos) const
	{
		return List::at(szPos);
	}

	inline typename List::value_type &Back(void) noexcept
	{
		return List::back();
	}

	inline const typename List::value_type &Back(void) const noexcept
	{
		return List::back();
	}

	//简化list插入
	template <class V>
	inline bool Add(size_t szPos, V &&vTagVal)
	{
		if (szPos > List::size())
		{
			return false;
		}
		if (!TestAndSetType(vTagVal.GetTag()))
		{
			return false;
		}

		List::emplace(List::begin() + szPos, std::forward<V>(vTagVal));
		return true;
	}

	template <class V>
	inline bool AddBack(V &&vTagVal)
	{
		if (!TestAndSetType(vTagVal.GetTag()))
		{
			return false;
		}

		List::emplace_back(std::forward<V>(vTagVal));
		return true;
	}

	template <class V>
	inline bool Set(size_t szPos, V &&vTagVal)
	{
		if (szPos > List::size())
		{
			return false;
		}
		if (!TestAndSetType(vTagVal.GetTag()))
		{
			return false;
		}

		List::operator[](szPos) = std::forward<V>(vTagVal);
		return true;
	}

	//简化删除
	inline bool Remove(size_t szPos)
	{
		if (szPos > List::size())
		{
			return false;
		}

		List::erase(List::begin() + szPos);
		return true;
	}

	inline void Clear(void)
	{
		List::clear();
	}

	//功能函数
	inline bool Empty(void) const noexcept
	{
		return List::empty();
	}

	inline size_t Size(void) const noexcept
	{
		return List::size();
	}

	inline void Reserve(size_t szNewCap)
	{
		return List::reserve(szNewCap);
	}

	inline void Resize(size_t szNewSize)
	{
		return List::resize(szNewSize);
	}

#define TYPE_GET_FUNC(type)\
inline const typename List::value_type::NBT_##type &Get##type(const typename List::size_type &szPos) const\
{\
	return List::at(szPos).Get##type();\
}\
\
inline typename List::value_type::NBT_##type &Get##type(const typename List::size_type &szPos)\
{\
	return List::at(szPos).Get##type();\
}\
\
inline const typename List::value_type::NBT_##type &Back##type(void) const\
{\
	return List::back().Get##type();\
}\
\
inline typename List::value_type::NBT_##type &Back##type(void)\
{\
	return List::back().Get##type();\
}

	TYPE_GET_FUNC(End);
	TYPE_GET_FUNC(Byte);
	TYPE_GET_FUNC(Short);
	TYPE_GET_FUNC(Int);
	TYPE_GET_FUNC(Long);
	TYPE_GET_FUNC(Float);
	TYPE_GET_FUNC(Double);
	TYPE_GET_FUNC(ByteArray);
	TYPE_GET_FUNC(IntArray);
	TYPE_GET_FUNC(LongArray);
	TYPE_GET_FUNC(String);
	TYPE_GET_FUNC(List);
	TYPE_GET_FUNC(Compound);

#undef TYPE_GET_FUNC

#define TYPE_PUT_FUNC(type)\
inline bool Add##type(size_t szPos, const typename List::value_type::NBT_##type &vTagVal)\
{\
	return Add(szPos, vTagVal);\
}\
\
inline bool Add##type(size_t szPos, typename List::value_type::NBT_##type &&vTagVal)\
{\
	return Add(szPos, vTagVal);\
}\
\
inline bool AddBack##type(const typename List::value_type::NBT_##type &vTagVal)\
{\
	return AddBack(vTagVal);\
}\
\
inline bool AddBack##type(typename List::value_type::NBT_##type &&vTagVal)\
{\
	return AddBack(vTagVal);\
}\
\
inline bool Set##type(size_t szPos, const typename List::value_type::NBT_##type &vTagVal)\
{\
	return Set(szPos, vTagVal);\
}\
\
inline bool Set##type(size_t szPos, typename List::value_type::NBT_##type &&vTagVal)\
{\
	return Set(szPos, vTagVal);\
}

	TYPE_PUT_FUNC(End);
	TYPE_PUT_FUNC(Byte);
	TYPE_PUT_FUNC(Short);
	TYPE_PUT_FUNC(Int);
	TYPE_PUT_FUNC(Long);
	TYPE_PUT_FUNC(Float);
	TYPE_PUT_FUNC(Double);
	TYPE_PUT_FUNC(ByteArray);
	TYPE_PUT_FUNC(IntArray);
	TYPE_PUT_FUNC(LongArray);
	TYPE_PUT_FUNC(String);
	TYPE_PUT_FUNC(List);
	TYPE_PUT_FUNC(Compound);

#undef TYPE_PUT_FUNC
};

#ifndef NBT_Node
#include "NBT_Node.hpp"//这个include用来骗IDE代码提示的
#endif