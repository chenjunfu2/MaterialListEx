#pragma once

#include <string>
#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"

template<typename String>
class MyString :public String
{
public:
	//�̳л��๹��
	using String::String;
};

//��std�����ռ�����Ӵ����hash�Ա�map�Զ���ȡ
namespace std
{
	template<typename T>
	struct hash<MyString<T>>
	{
		size_t operator()(const MyString<T> &s) const noexcept
		{
			return std::hash<T>{}(s);
		}
	};
}


#ifndef NBT_Node
#include "NBT_Node.hpp"//���include����ƭIDE������ʾ��
#endif