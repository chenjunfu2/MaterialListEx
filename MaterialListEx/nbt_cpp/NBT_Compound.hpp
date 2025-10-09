#pragma once

#include <unordered_map>
#include <compare>
#include <type_traits>
#include <initializer_list>

#include "NBT_Type.hpp"

class NBT_Reader;
class NBT_Writer;

template <typename T>
concept HasSpaceship = requires(const T & a, const T & b)
{
	{ a <=> b };
};

template <typename T>
concept HasRBegin = requires(T t) {t.rbegin();};
template <typename T>
concept HasCRBegin = requires(T t) {t.crbegin();};
template <typename T>
concept HasREnd = requires(T t) {t.rend();};
template <typename T>
concept HasCREnd = requires(T t) {t.crend();};

template<typename Compound>
class MyCompound :protected Compound//Compound is Map
{
	friend class NBT_Reader;
	friend class NBT_Writer;

private:
	//总是允许插入nbt end，但是在写出文件时会忽略end类型
	//template<typename V>
	//bool TestType(V vTagVal)
	//{
	//	if constexpr (std::is_same_v<std::decay_t<V>, Compound::mapped_type>)
	//	{
	//		return vTagVal.GetTag() != NBT_TAG::End;
	//	}
	//	else
	//	{
	//		return NBT_Type::TypeTag_V<std::decay_t<V>> != NBT_TAG::End;
	//	}
	//}
public:
	//完美转发、初始化列表代理构造
	template<typename... Args>
	MyCompound(Args&&... args) : Compound(std::forward<Args>(args)...)
	{}

	MyCompound(std::initializer_list<typename Compound::value_type> init) : Compound(init)
	{}

	//无参构造析构
	MyCompound(void) = default;
	~MyCompound(void) = default;

	//移动拷贝构造
	MyCompound(MyCompound &&_Move) noexcept :Compound(std::move(_Move))
	{}

	MyCompound(const MyCompound &_Copy) noexcept :Compound(_Copy)
	{}

	//赋值
	MyCompound &operator=(MyCompound &&_Move) noexcept
	{
		Compound::operator=(std::move(_Move));
		return *this;
	}

	MyCompound &operator=(const MyCompound &_Copy)
	{
		Compound::operator=(_Copy);
		return *this;
	}


	//返回内部数据（父类）
	const Compound &GetData(void) const noexcept
	{
		return *this;
	}

	//运算符重载
	bool operator==(const MyCompound &_Right) const noexcept
	{
		return (const Compound &)*this == (const Compound &)_Right;
	}

	bool operator!=(const MyCompound &_Right) const noexcept
	{
		return (const Compound &)*this != (const Compound &)_Right;
	}

	std::partial_ordering operator<=>(const MyCompound &_Right) const noexcept
	{
		if constexpr (HasSpaceship<Compound>)
		{
			return (const Compound &)*this <=> (const Compound &)_Right;
		}
		else
		{
			if (operator==(_Right))
			{
				return std::partial_ordering::equivalent;//相等返回相等
			}
			else
			{
				return std::partial_ordering::unordered;//否则返回不可比较
			}
		}
	}

	//暴露父类接口
	using Compound::begin;
	using Compound::end;
	using Compound::cbegin;
	using Compound::cend;
	using Compound::operator[];

	//using Compound::rbegin;
	//using Compound::rend;
	//using Compound::crbegin;
	//using Compound::crend;
	
	//存在则映射
	//-------------------- rbegin --------------------
	auto rbegin() requires HasRBegin<Compound> {return Compound::rbegin();}
	auto rbegin() const requires HasRBegin<Compound> {return Compound::rbegin();}
	auto crbegin() const noexcept requires HasCRBegin<Compound> {return Compound::crbegin();}
	//-------------------- rend --------------------
	auto rend() requires HasREnd<Compound> {return Compound::rend();}
	auto rend() const requires HasREnd<Compound> {return Compound::rend();}
	auto crend() const noexcept requires HasCREnd<Compound> {return Compound::crend();}
	
	

	//简化map查询
	typename Compound::mapped_type &Get(const typename Compound::key_type &sTagName)
	{
		return Compound::at(sTagName);
	}

	const typename Compound::mapped_type &Get(const typename Compound::key_type &sTagName) const
	{
		return Compound::at(sTagName);
	}

	typename Compound::mapped_type *Search(const typename Compound::key_type &sTagName) noexcept
	{
		auto find = Compound::find(sTagName);
		return find == Compound::end() ? NULL : &((*find).second);
	}

	const typename Compound::mapped_type *Search(const typename Compound::key_type &sTagName) const noexcept
	{
		auto find = Compound::find(sTagName);
		return find == Compound::end() ? NULL : &((*find).second);
	}

	//简化map插入
	//使用完美转发，不丢失引用、右值信息
	template <typename K, typename V>
	std::pair<typename Compound::iterator, bool> Put(K &&sTagName, V &&vTagVal)
		requires std::constructible_from<typename Compound::key_type, K &&> && std::constructible_from<typename Compound::mapped_type, V &&>
	{
		//总是允许插入nbt end，但是在写出文件时会忽略end类型
		//if (!TestType(vTagVal))
		//{
		//	return std::pair{ Compound::end(),false };
		//}

		return Compound::try_emplace(std::forward<K>(sTagName), std::forward<V>(vTagVal));
	}

	std::pair<typename Compound::iterator, bool> Put(const typename Compound::value_type &mapValue)
	{
		//总是允许插入nbt end，但是在写出文件时会忽略end类型
		//if (!TestType(mapValue.second.GetTag()))
		//{
		//	return std::pair{ Compound::end(),false };
		//}

		return Compound::insert(mapValue);
	}

	std::pair<typename Compound::iterator, bool> Put(typename Compound::value_type &&mapValue)
	{
		//总是允许插入nbt end，但是在写出文件时会忽略end类型
		//if (!TestType(mapValue.second.GetTag()))
		//{
		//	return std::pair{ Compound::end(),false };
		//}

		return Compound::insert(std::move(mapValue));
	}

	//简化删除
	bool Remove(const typename Compound::key_type &sTagName)
	{
		return Compound::erase(sTagName) != 0;//返回1即为成功，否则为0，标准库：返回值为删除的元素数（0 或 1）。
	}

	void Clear(void)
	{
		Compound::clear();
	}

	//功能函数
	bool Empty(void) const noexcept
	{
		return Compound::empty();
	}

	size_t Size(void) const noexcept
	{
		return Compound::size();
	}

	//简化判断
	bool Contains(const typename Compound::key_type &sTagName) const noexcept
	{
		return Compound::contains(sTagName);
	}

	bool Contains(const typename Compound::key_type &sTagName, const NBT_TAG &enTypeTag) const noexcept
	{
		auto *p = Search(sTagName);
		return p != NULL && p->GetTag() == enTypeTag;
	}

#define TYPE_GET_FUNC(type)\
const typename NBT_Type::##type &Get##type(const typename Compound::key_type & sTagName) const\
{\
	return Compound::at(sTagName).Get##type();\
}\
\
typename NBT_Type::##type &Get##type(const typename Compound::key_type & sTagName)\
{\
	return Compound::at(sTagName).Get##type();\
}\
\
const typename NBT_Type::##type *Has##type(const typename Compound::key_type & sTagName) const noexcept\
{\
	auto find = Compound::find(sTagName);\
	return find != Compound::end() && find->second.Is##type() ? &(find->second.Get##type()) : NULL;\
}\
\
typename NBT_Type::##type *Has##type(const typename Compound::key_type & sTagName) noexcept\
{\
	auto find = Compound::find(sTagName);\
	return find != Compound::end() && find->second.Is##type() ? &(find->second.Get##type()) : NULL;\
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
template <typename K>\
std::pair<typename Compound::iterator, bool> Put##type(K &&sTagName, const typename NBT_Type::##type &vTagVal)\
	requires std::constructible_from<typename Compound::key_type, K &&>\
{\
	return Put(std::forward<K>(sTagName), vTagVal);\
}\
\
template <typename K>\
std::pair<typename Compound::iterator, bool> Put##type(K &&sTagName, typename NBT_Type::##type &&vTagVal)\
	requires std::constructible_from<typename Compound::key_type, K &&>\
{\
	return Put(std::forward<K>(sTagName), std::move(vTagVal));\
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