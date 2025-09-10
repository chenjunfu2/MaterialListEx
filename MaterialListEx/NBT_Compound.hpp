#pragma once

#include <map>
#include <compare>
#include <type_traits>

#include "NBT_TAG.hpp"

template<typename Map>
class MyCompound :public Map
{
public:
	//�̳л��๹��
	using Map::Map;

	//��map��ѯ
	inline typename Map::mapped_type &Get(const typename Map::key_type &sTagName)
	{
		return Map::at(sTagName);
	}

	inline const typename Map::mapped_type &Get(const typename Map::key_type &sTagName) const
	{
		return Map::at(sTagName);
	}

	inline typename Map::mapped_type *Search(const typename Map::key_type &sTagName) noexcept
	{
		auto find = Map::find(sTagName);
		return find == Map::end() ? NULL : &((*find).second);
	}

	inline const typename Map::mapped_type *Search(const typename Map::key_type &sTagName) const noexcept
	{
		auto find = Map::find(sTagName);
		return find == Map::end() ? NULL : &((*find).second);
	}

	//��map����
	//ʹ������ת��������ʧ���á���ֵ��Ϣ
	template <class K, class V>
	inline std::pair<typename Map::iterator, bool> Put(K &&sTagName, V &&vTagVal)
		requires std::constructible_from<typename Map::key_type, K &&> && std::constructible_from<typename Map::mapped_type, V &&>
	{
		return Map::try_emplace(std::forward<K>(sTagName), std::forward<V>(vTagVal));
	}

	inline std::pair<typename Map::iterator, bool> Put(const typename Map::value_type &mapValue)
	{
		return Map::insert(mapValue);
	}

	inline std::pair<typename Map::iterator, bool> Put(typename Map::value_type &&mapValue)
	{
		return Map::insert(std::move(mapValue));
	}

	//��ɾ��
	inline bool Remove(const typename Map::key_type &sTagName)
	{
		return Map::erase(sTagName) != 0;//����1��Ϊ�ɹ�������Ϊ0����׼�⣺����ֵΪɾ����Ԫ������0 �� 1����
	}

	//���ܺ���
	inline bool Empty(void) const noexcept
	{
		return Map::empty();
	}

	inline size_t Size(void) const noexcept
	{
		return Map::size();
	}

	//���ж�
	inline bool Contains(const typename Map::key_type &sTagName) const noexcept
	{
		return Map::contains(sTagName);
	}

	inline bool Contains(const typename Map::key_type &sTagName, const NBT_TAG &enTypeTag) const noexcept
	{
		auto *p = Search(sTagName);
		return p != NULL && p->GetTag() == enTypeTag;
	}

#define TYPE_GET_FUNC(type)\
inline const typename Map::mapped_type::NBT_##type &Get##type(const typename Map::key_type & sTagName) const\
{\
	return Map::at(sTagName).Get##type();\
}\
\
inline typename Map::mapped_type::NBT_##type &Get##type(const typename Map::key_type & sTagName)\
{\
	return Map::at(sTagName).Get##type();\
}\
\
inline const typename Map::mapped_type::NBT_##type *Has##type(const typename Map::key_type & sTagName) const noexcept\
{\
	auto find = Map::find(sTagName);\
	return find != Map::end() && find->second.Is##type() ? &(find->second.Get##type()) : NULL;\
}\
\
inline typename Map::mapped_type::NBT_##type *Has##type(const typename Map::key_type & sTagName) noexcept\
{\
	auto find = Map::find(sTagName);\
	return find != Map::end() && find->second.Is##type() ? &(find->second.Get##type()) : NULL;\
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
template <class K>\
inline std::pair<typename Map::iterator, bool> Put##type(K &&sTagName, const typename Map::mapped_type::NBT_##type &vTagVal)\
	requires std::constructible_from<typename Map::key_type, K &&>\
{\
	return Put(std::forward<K>(sTagName), vTagVal);\
}\
\
template <class K>\
inline std::pair<typename Map::iterator, bool> Put##type(K &&sTagName, typename Map::mapped_type::NBT_##type &&vTagVal)\
	requires std::constructible_from<typename Map::key_type, K &&>\
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