#pragma once

#include <vector>
#include <compare>
#include <type_traits>
#include <initializer_list>
#include <stdexcept>

#include "NBT_Type.hpp"

template <typename DataType>
class NBT_Reader;

template <typename DataType>
class NBT_Writer;

template <typename List>
class MyList :protected List
{
	template <typename DataType>
	friend class NBT_Reader;

	template <typename DataType>
	class NBT_Writer;
private:
	//列表元素类型（只能一种元素）
	NBT_TAG enValueTag = NBT_TAG::End;
	//此变量仅用于规约列表元素类型，无需参与比较与hash，但是需要参与构造与移动
private:
	inline bool TestTagAndSetType(NBT_TAG enTargetTag)
	{
		if (enTargetTag == NBT_TAG::End)
		{
			return false;//不能插入空值，拒掉
		}

		if (enValueTag == NBT_TAG::End)
		{
			enValueTag = enTargetTag;
			return true;
		}

		return enTargetTag == enValueTag;
	}

	template<typename V>
	inline bool TestAndSetType(const V &vTagVal)
	{
		if constexpr (std::is_same_v<std::decay_t<V>, List::value_type>)
		{
			return TestTagAndSetType(vTagVal.GetTag());
		}
		else
		{
			return TestTagAndSetType(NBT_Type::TypeTag_V<std::decay_t<V>>);
		}
	}

	inline void TestInit(void)
	{
		if (enValueTag == NBT_TAG::End && !List::empty())
		{
			Clear();
			throw std::invalid_argument("Tag is end but the list is not empty");
		}

		if (enValueTag != NBT_TAG::End)
		{
			for (const auto &it : (List)*this)
			{
				if (it.GetTag() != enValueTag)
				{
					Clear();
					throw std::invalid_argument("List contains elements of different types");
				}
			}
		}
	}
public:
	template<typename... Args>
	MyList(NBT_TAG _enValueTag, Args&&... args) :List(std::forward<Args>(args)...), enValueTag(_enValueTag)
	{
		TestInit();
	}

	template<typename... Args>
	MyList(Args&&... args) : List(std::forward<Args>(args)...), enValueTag(List::empty() ? NBT_TAG::End : List::front().GetTag())
	{
		TestInit();
	}

	MyList(NBT_TAG _enValueTag, std::initializer_list<typename List::value_type> init) : List(init), enValueTag(_enValueTag)
	{
		TestInit();
	}

	MyList(std::initializer_list<typename List::value_type> init) : List(init), enValueTag(List::empty() ? NBT_TAG::End : List::front().GetTag())
	{
		TestInit();
	}

	MyList(void) = default;

	~MyList(void)
	{
		Clear();
	}

	MyList(MyList &&_Move) noexcept
		:List(std::move(_Move)),
		 enValueTag(std::move(_Move.enValueTag))
	{
		_Move.enValueTag = NBT_TAG::End;
	}
	MyList(const MyList &_Other)
		:List(_Other),
		enValueTag(_Other.enValueTag)
	{}

	MyList &operator=(MyList &&_Move) noexcept
	{
		List::operator=(std::move(_Move));
		enValueTag = std::move(_Move.enValueTag);

		_Move.enValueTag = NBT_TAG::End;

		return *this;
	}

	MyList &operator=(const MyList &_Other)
	{
		List::operator=(_Other);
		enValueTag = _Other.enValueTag;

		return *this;
	}

	//运算符重载
	bool operator==(const MyList &_Right) const noexcept
	{
		return enValueTag == _Right.enValueTag &&
			(const List &)*this == (const List &)_Right;
	}

	bool operator!=(const MyList &_Right) const noexcept
	{
		return enValueTag != _Right.enValueTag ||
			(const List &)*this != (const List &)_Right;
	}

	std::partial_ordering operator<=>(const MyList &_Right) const noexcept
	{
		if (auto cmp = enValueTag <=> _Right.enValueTag; cmp != 0)
		{
			return cmp;
		}

		return (const List &)*this <=> (const List &)_Right;
	}

	//暴露父类接口
	using List::begin;
	using List::end;
	using List::cbegin;
	using List::cend;
	using List::rbegin;
	using List::rend;
	using List::crbegin;
	using List::crend;
	using List::operator[];//所有使用返回引用的api并修改list内部variant类型的操作都是未定义行为

	//自定义操作
	bool SetTag(NBT_TAG tagNewValue)
	{
		if (!List::empty())
		{
			return false;
		}

		enValueTag = tagNewValue;
		return true;
	}

	NBT_TAG GetTag(void)
	{
		return enValueTag;
	}

	//简化list查询
	//所有使用返回引用的api并修改list内部variant类型的操作都是未定义行为
	inline typename List::value_type &Get(const typename List::size_type &szPos)
	{
		return List::at(szPos);
	}

	inline const typename List::value_type &Get(const typename List::size_type &szPos) const
	{
		return List::at(szPos);
	}

	inline typename List::value_type &Front(void) noexcept
	{
		return List::front();
	}

	inline const typename List::value_type &Front(void) const noexcept
	{
		return List::front();
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
	template <typename V>
	inline std::pair<typename List::iterator, bool> Add(size_t szPos, V &&vTagVal)
	{
		if (szPos > List::size())
		{
			return std::pair{ List::end(),false };
		}
		if (!TestAndSetType(vTagVal))
		{
			return std::pair{ List::end(),false };
		}

		return std::pair{ List::emplace(List::begin() + szPos, std::forward<V>(vTagVal)),true };
	}

	template <typename V>
	inline std::pair<typename List::iterator, bool> AddBack(V &&vTagVal)
	{
		if (!TestAndSetType(vTagVal))
		{
			return std::pair{ List::end(),false };
		}

		List::emplace_back(std::forward<V>(vTagVal));
		return std::pair{ List::end() - 1,true };
	}

	template <typename V>
	inline std::pair<typename List::iterator, bool> Set(size_t szPos, V &&vTagVal)
	{
		if (szPos > List::size())
		{
			return std::pair{ List::end(),false };
		}
		if (!TestAndSetType(vTagVal))
		{
			return std::pair{ List::end(),false };
		}

		List::operator[](szPos) = std::forward<V>(vTagVal);
		return std::pair{ List::begin() + szPos,true };
	}

	//简化删除
	inline bool Remove(size_t szPos)
	{
		if (szPos > List::size())
		{
			return false;
		}

		List::erase(List::begin() + szPos);//这个没必要返回结果，直接丢弃

		if (Empty())
		{
			enValueTag = NBT_TAG::End;//清除类型
		}

		return true;
	}

	inline void Clear(void)
	{
		List::clear();
		enValueTag = NBT_TAG::End;
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
inline const typename NBT_Type::##type &Get##type(const typename List::size_type &szPos) const\
{\
	return List::at(szPos).Get##type();\
}\
\
inline typename NBT_Type::##type &Get##type(const typename List::size_type &szPos)\
{\
	return List::at(szPos).Get##type();\
}\
\
inline const typename NBT_Type::##type &Back##type(void) const\
{\
	return List::back().Get##type();\
}\
\
inline typename NBT_Type::##type &Back##type(void)\
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
inline std::pair<typename List::iterator, bool> Add##type(size_t szPos, const typename NBT_Type::##type &vTagVal)\
{\
	return Add(szPos, vTagVal);\
}\
\
inline std::pair<typename List::iterator, bool> Add##type(size_t szPos, typename NBT_Type::##type &&vTagVal)\
{\
	return Add(szPos, vTagVal);\
}\
\
inline std::pair<typename List::iterator, bool> AddBack##type(const typename NBT_Type::##type &vTagVal)\
{\
	return AddBack(vTagVal);\
}\
\
inline std::pair<typename List::iterator, bool> AddBack##type(typename NBT_Type::##type &&vTagVal)\
{\
	return AddBack(vTagVal);\
}\
\
inline std::pair<typename List::iterator, bool> Set##type(size_t szPos, const typename NBT_Type::##type &vTagVal)\
{\
	return Set(szPos, vTagVal);\
}\
\
inline std::pair<typename List::iterator, bool> Set##type(size_t szPos, typename NBT_Type::##type &&vTagVal)\
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