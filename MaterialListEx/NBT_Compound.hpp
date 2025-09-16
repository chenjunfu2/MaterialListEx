#pragma once

#include <map>
#include <compare>
#include <type_traits>
#include <initializer_list>

#include "NBT_Type.hpp"

template <typename DataType>
class NBT_Reader;

template <typename DataType>
class NBT_Writer;

template<typename Compound>
class MyCompound :protected Compound//Compound is Map
{
	template <typename DataType>
	friend class NBT_Reader;

	template <typename DataType>
	class NBT_Writer;
private:
	//�����������nbt end��������д���ļ�ʱ�����end����
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
	//����ת������ʼ���б������
	template<typename... Args>
	MyCompound(Args&&... args) : Compound(std::forward<Args>(args)...)
	{}

	MyCompound(std::initializer_list<typename Compound::value_type> init) : Compound(init)
	{}

	//�޲ι�������
	MyCompound(void) = default;
	~MyCompound(void) = default;

	//�ƶ���������
	MyCompound(MyCompound &&_Move) noexcept :Compound(std::move(_Move))
	{}

	MyCompound(const MyCompound &_Copy) noexcept :Compound(_Copy)
	{}

	//��ֵ
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


	//�����ڲ����ݣ����ࣩ
	const Compound &GetData(void) const noexcept
	{
		return *this;
	}

	//���������
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
		return (const Compound &)*this <=> (const Compound &)_Right;
	}

	//��¶����ӿ�
	using Compound::begin;
	using Compound::end;
	using Compound::cbegin;
	using Compound::cend;
	using Compound::rbegin;
	using Compound::rend;
	using Compound::crbegin;
	using Compound::crend;
	using Compound::operator[];

	//��map��ѯ
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

	//��map����
	//ʹ������ת��������ʧ���á���ֵ��Ϣ
	template <typename K, typename V>
	std::pair<typename Compound::iterator, bool> Put(K &&sTagName, V &&vTagVal)
		requires std::constructible_from<typename Compound::key_type, K &&> && std::constructible_from<typename Compound::mapped_type, V &&>
	{
		//�����������nbt end��������д���ļ�ʱ�����end����
		//if (!TestType(vTagVal))
		//{
		//	return std::pair{ Compound::end(),false };
		//}

		return Compound::try_emplace(std::forward<K>(sTagName), std::forward<V>(vTagVal));
	}

	std::pair<typename Compound::iterator, bool> Put(const typename Compound::value_type &mapValue)
	{
		//�����������nbt end��������д���ļ�ʱ�����end����
		//if (!TestType(mapValue.second.GetTag()))
		//{
		//	return std::pair{ Compound::end(),false };
		//}

		return Compound::insert(mapValue);
	}

	std::pair<typename Compound::iterator, bool> Put(typename Compound::value_type &&mapValue)
	{
		//�����������nbt end��������д���ļ�ʱ�����end����
		//if (!TestType(mapValue.second.GetTag()))
		//{
		//	return std::pair{ Compound::end(),false };
		//}

		return Compound::insert(std::move(mapValue));
	}

	//��ɾ��
	bool Remove(const typename Compound::key_type &sTagName)
	{
		return Compound::erase(sTagName) != 0;//����1��Ϊ�ɹ�������Ϊ0����׼�⣺����ֵΪɾ����Ԫ������0 �� 1����
	}

	void Clear(void)
	{
		Compound::clear();
	}

	//���ܺ���
	bool Empty(void) const noexcept
	{
		return Compound::empty();
	}

	size_t Size(void) const noexcept
	{
		return Compound::size();
	}

	//���ж�
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
	return Put(std::forward<K>(sTagName), vTagVal);\
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
#include "NBT_Node.hpp"//���include����ƭIDE������ʾ��
#endif