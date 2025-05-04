#pragma once

//用于将带Tag的Item转换到唯一的ItemName，以便进行统计

#include "NBT_Node.hpp"

void AddSwitch(NBT_Node::NBT_String &sRet, const NBT_Node &nNode, uint64_t szStackDepth)
{
	if (szStackDepth <= 0)
	{
		return;
	}







}

NBT_Node::NBT_String GetTagItemName(const NBT_Node::NBT_String &sName, const NBT_Node::NBT_Compound cpdTag, uint64_t szStackDepth = 64)
{
	NBT_Node::NBT_String sRet{ sName };
	sRet += '{';

	for (auto &it : cpdTag)
	{
		sRet += it.first;
		
		AddSwitch(sRet, it.second, szStackDepth - 1);

		sRet += ',';
	}

	if (cpdTag.size() != 0)
	{
		sRet.pop_back();//清除最后一个逗号
	}


	sRet += '}';
	return sRet;
}