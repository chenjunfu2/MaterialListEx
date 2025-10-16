#pragma once

#include <vector>
#include <compare>
#include <type_traits>
#include <initializer_list>
#include <stdexcept>

#include "NBT_Type.hpp"

class NBT_Reader;
class NBT_Writer;

template <typename List>
class MyList :protected List
{
	friend class NBT_Reader;
	friend class NBT_Writer;

private:
	//列表元素类型（只能一种元素）
	NBT_TAG enElementTag = NBT_TAG::End;
	//此变量仅用于规约列表元素类型，无需参与比较与hash，但是需要参与构造与移动
private:
	bool TestTagAndSetType(NBT_TAG enTargetTag)
	{
		if (enTargetTag == NBT_TAG::End)
		{
			return false;//不能插入空值，拒掉
		}

		if (enElementTag == NBT_TAG::End)
		{
			enElementTag = enTargetTag;
			return true;
		}

		return enTargetTag == enElementTag;
	}

	template<typename V>
	bool TestAndSetType(const V &vTagVal)
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

	void TestInit(void)
	{
		if (enElementTag == NBT_TAG::End && !List::empty())
		{
			Clear();
			throw std::invalid_argument("Tag is end but the list is not empty");
		}

		if (enElementTag != NBT_TAG::End)
		{
			for (const auto &it : (List)*this)
			{
				if (it.GetTag() != enElementTag)
				{
					Clear();
					throw std::invalid_argument("List contains elements of different types");
				}
			}
		}
	}
public:
	//提示性标签，用于取消构造函数的检测
	struct NoCheck_T
	{
		explicit constexpr NoCheck_T() noexcept = default;
	};
	//方便用户传入的参数
	constexpr static inline const NoCheck_T NoCheck{};

	//完美转发、初始化列表代理构造
	template<typename... Args>
	MyList(NBT_TAG _enElementTag, Args&&... args) :List(std::forward<Args>(args)...), enElementTag(_enElementTag)
	{
		TestInit();
	}

	template<typename... Args>
	MyList(NoCheck_T, NBT_TAG _enElementTag, Args&&... args) : List(std::forward<Args>(args)...), enElementTag(_enElementTag)
	{}

	template<typename... Args>
	MyList(Args&&... args) : List(std::forward<Args>(args)...), enElementTag(List::empty() ? NBT_TAG::End : List::front().GetTag())
	{
		TestInit();
	}

	template<typename... Args>
	MyList(NoCheck_T, Args&&... args) : List(std::forward<Args>(args)...), enElementTag(List::empty() ? NBT_TAG::End : List::front().GetTag())
	{}

	MyList(NBT_TAG _enElementTag, std::initializer_list<typename List::value_type> init) : List(init), enElementTag(_enElementTag)
	{
		TestInit();
	}

	MyList(NoCheck_T, NBT_TAG _enElementTag, std::initializer_list<typename List::value_type> init) : List(init), enElementTag(_enElementTag)
	{}

	MyList(std::initializer_list<typename List::value_type> init) : List(init), enElementTag(List::empty() ? NBT_TAG::End : List::front().GetTag())
	{
		TestInit();
	}

	MyList(NoCheck_T, std::initializer_list<typename List::value_type> init) : List(init), enElementTag(List::empty() ? NBT_TAG::End : List::front().GetTag())
	{}

	//无参构造析构
	MyList(void) = default;
	~MyList(void)
	{
		Clear();
	}

	//移动拷贝构造
	MyList(MyList &&_Move) noexcept
		:List(std::move(_Move)),
		 enElementTag(std::move(_Move.enElementTag))
	{
		_Move.enElementTag = NBT_TAG::End;
	}
	MyList(const MyList &_Copy)
		:List(_Copy),
		enElementTag(_Copy.enElementTag)
	{}

	//赋值
	MyList &operator=(MyList &&_Move) noexcept
	{
		List::operator=(std::move(_Move));
		enElementTag = std::move(_Move.enElementTag);

		_Move.enElementTag = NBT_TAG::End;

		return *this;
	}

	MyList &operator=(const MyList &_Copy)
	{
		List::operator=(_Copy);
		enElementTag = _Copy.enElementTag;

		return *this;
	}

	//返回内部数据（父类）
	const List &GetData(void) const noexcept
	{
		return *this;
	}

	//运算符重载
	bool operator==(const MyList &_Right) const noexcept
	{
		return enElementTag == _Right.enElementTag &&
			(const List &)*this == (const List &)_Right;
	}

	bool operator!=(const MyList &_Right) const noexcept
	{
		return enElementTag != _Right.enElementTag ||
			(const List &)*this != (const List &)_Right;
	}

	std::partial_ordering operator<=>(const MyList &_Right) const noexcept
	{
		if (auto cmp = enElementTag <=> _Right.enElementTag; cmp != 0)
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
	template <bool bNoCheck = false>
	std::conditional_t<bNoCheck, void, bool> SetTag(NBT_TAG tagNewValue)
	{
		if (!bNoCheck)
		{
			if (!List::empty())
			{
				return false;
			}
		}

		enElementTag = tagNewValue;

		if (!bNoCheck)
		{
			return true;
		}
	}

	NBT_TAG GetTag(void)
	{
		return enElementTag;
	}

	//简化list查询
	//所有使用返回引用的api并修改list内部variant类型的操作都是未定义行为
	typename List::value_type &Get(const typename List::size_type &szPos)
	{
		return List::at(szPos);
	}

	const typename List::value_type &Get(const typename List::size_type &szPos) const
	{
		return List::at(szPos);
	}

	typename List::value_type &Front(void) noexcept
	{
		return List::front();
	}

	const typename List::value_type &Front(void) const noexcept
	{
		return List::front();
	}

	typename List::value_type &Back(void) noexcept
	{
		return List::back();
	}

	const typename List::value_type &Back(void) const noexcept
	{
		return List::back();
	}

	//简化list插入
	template <bool bNoCheck = false, typename V>
	std::conditional_t<bNoCheck, typename List::iterator, std::pair<typename List::iterator, bool>> Add(size_t szPos, V &&vTagVal)
	{
		if (!bNoCheck)
		{
			if (szPos > List::size())
			{
				return std::pair{ List::end(),false };
			}
			if (!TestAndSetType(vTagVal))
			{
				return std::pair{ List::end(),false };
			}
		}

		//插入
		typename List::iterator itRet = List::emplace(List::begin() + szPos, std::forward<V>(vTagVal));

		if (!bNoCheck)
		{
			return std::pair{ itRet,true };
		}
		else
		{
			return itRet;
		}
	}

	template <bool bNoCheck = false, typename V>
	std::conditional_t<bNoCheck, typename List::iterator, std::pair<typename List::iterator, bool>> AddBack(V &&vTagVal)
	{
		if constexpr (!bNoCheck)
		{
			if (!TestAndSetType(vTagVal))
			{
				return std::pair{ List::end(),false };
			}
		}

		//插入
		List::emplace_back(std::forward<V>(vTagVal));

		if constexpr (!bNoCheck)
		{
			return std::pair{ List::end() - 1,true };
		}
		else
		{
			return List::end() - 1;
		}
	}

	template <bool bNoCheck = false, typename V>
	std::conditional_t<bNoCheck, typename List::iterator, std::pair<typename List::iterator, bool>> Set(size_t szPos, V &&vTagVal)
	{
		if constexpr (!bNoCheck)
		{
			if (szPos > List::size())
			{
				return std::pair{ List::end(),false };
			}
			if (!TestAndSetType(vTagVal))
			{
				return std::pair{ List::end(),false };
			}
		}

		List::operator[](szPos) = std::forward<V>(vTagVal);

		if constexpr (!bNoCheck)
		{
			return std::pair{ List::begin() + szPos,true };
		}
		else
		{
			return List::begin() + szPos;
		}
	}

	//简化删除
	template <bool bNoCheck = false>
	std::conditional_t<bNoCheck, void, bool> Remove(size_t szPos)
	{
		if constexpr (!bNoCheck)
		{
			if (szPos > List::size())
			{
				return false;
			}
		}

		List::erase(List::begin() + szPos);//这个没必要返回结果，直接丢弃

		if (Empty())
		{
			enElementTag = NBT_TAG::End;//清除类型
		}

		if constexpr (!bNoCheck)
		{
			return true;
		}
	}

	void Clear(void)
	{
		List::clear();
		enElementTag = NBT_TAG::End;
	}

	//功能函数
	bool Empty(void) const noexcept
	{
		return List::empty();
	}

	size_t Size(void) const noexcept
	{
		return List::size();
	}

	void Reserve(size_t szNewCap)
	{
		return List::reserve(szNewCap);
	}

	void Resize(size_t szNewSize)
	{
		return List::resize(szNewSize);
	}

	void ShrinkToFit(void)
	{
		return List::shrink_to_fit();
	}

	template <bool bNoCheck = false>
	std::conditional_t<bNoCheck, void, bool> Merge(const MyList &_Copy)
	{
		if constexpr (!bNoCheck)
		{
			if (_Copy.enElementTag != enElementTag)
			{
				return false;
			}
		}

		List::insert(List::end(), _Copy.begin(), _Copy.end());

		if constexpr (!bNoCheck)
		{
			return true;
		}
	}

	template <bool bNoCheck = false>
	std::conditional_t<bNoCheck, void, bool> Merge(MyList &&_Move)
	{
		if constexpr (!bNoCheck)
		{
			if (_Move.enElementTag != enElementTag)
			{
				return false;
			}
		}

		List::insert(List::end(), std::make_move_iterator(_Move.begin()), std::make_move_iterator(_Move.end()));

		if constexpr (!bNoCheck)
		{
			return true;
		}
	}

	//简化判断
	bool Contains(const typename List::value_type &tValue) const noexcept
	{
		return std::find(List::begin(), List::end(), tValue) != List::end();
	}

	template<typename Predicate>
	bool ContainsIf(Predicate pred) const noexcept
	{
		return std::find_if(List::begin(), List::end(), pred) != List::end();
	}

#define TYPE_GET_FUNC(type)\
const typename NBT_Type::type &Get##type(const typename List::size_type &szPos) const\
{\
	return List::at(szPos).Get##type();\
}\
\
typename NBT_Type::type &Get##type(const typename List::size_type &szPos)\
{\
	return List::at(szPos).Get##type();\
}\
\
const typename NBT_Type::type &Back##type(void) const\
{\
	return List::back().Get##type();\
}\
\
typename NBT_Type::type &Back##type(void)\
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
template <bool bNoCheck = false>\
std::conditional_t<bNoCheck, typename List::iterator, std::pair<typename List::iterator, bool>> Add##type(size_t szPos, const typename NBT_Type::type &vTagVal)\
{\
	return Add<bNoCheck>(szPos, vTagVal);\
}\
\
template <bool bNoCheck = false>\
std::conditional_t<bNoCheck, typename List::iterator, std::pair<typename List::iterator, bool>> Add##type(size_t szPos, typename NBT_Type::type &&vTagVal)\
{\
	return Add<bNoCheck>(szPos, std::move(vTagVal));\
}\
\
template <bool bNoCheck = false>\
std::conditional_t<bNoCheck, typename List::iterator, std::pair<typename List::iterator, bool>> AddBack##type(const typename NBT_Type::type &vTagVal)\
{\
	return AddBack<bNoCheck>(vTagVal);\
}\
\
template <bool bNoCheck = false>\
std::conditional_t<bNoCheck, typename List::iterator, std::pair<typename List::iterator, bool>> AddBack##type(typename NBT_Type::type &&vTagVal)\
{\
	return AddBack<bNoCheck>(std::move(vTagVal));\
}\
\
template <bool bNoCheck = false>\
std::conditional_t<bNoCheck, typename List::iterator, std::pair<typename List::iterator, bool>> Set##type(size_t szPos, const typename NBT_Type::type &vTagVal)\
{\
	return Set<bNoCheck>(szPos, vTagVal);\
}\
\
template <bool bNoCheck = false>\
std::conditional_t<bNoCheck, typename List::iterator, std::pair<typename List::iterator, bool>> Set##type(size_t szPos, typename NBT_Type::type &&vTagVal)\
{\
	return Set<bNoCheck>(szPos, std::move(vTagVal));\
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