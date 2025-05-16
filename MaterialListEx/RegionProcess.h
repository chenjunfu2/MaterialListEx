#pragma once

#include <algorithm>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>

#include "NBT_Node.hpp"
#include "ItemProcess.hpp"

template<typename Map>
struct MapSortList
{
	using MapPair = Map::value_type;

	//map���ڲ���ͳ�ƣ�����Ҫ�����key�����ң�����key��ʲô����ν
	Map mapItemCounter{};//��������״̬����Ʒӳ��map
	//ͨ��vector����map��Ԫ�����򣨴洢pair���ã�std::reference_wrapper<>
	std::vector<std::reference_wrapper<MapPair>> vecSortItem{};

	static bool SortCmp(MapPair &l, MapPair &r)
	{
		if (l.second == r.second)//�����������°�key����
		{
			return l.first < r.first;//����
		}
		else//����val����
		{
			return l.second > r.second;//����
		}
	}
	
	void SortElement(void)
	{
		//��ǰ���ݼ��ٲ��뿪��
		vecSortItem.reserve(mapItemCounter.size());
		vecSortItem.assign(mapItemCounter.begin(), mapItemCounter.end());//��������Χ����
		//����Ʒ��������������
		std::sort(vecSortItem.begin(), vecSortItem.end(), SortCmp);
	}
};



struct RegionStats
{
	NBT_Node::NBT_String sRegionName{};
	MapSortList<std::unordered_map<Item, uint64_t, decltype(&Item::Hash), decltype(&Item::Equal)>> mslBlock{ .mapItemCounter{128, &Item::Hash, &Item::Equal} };
	//MapSortList<std::map<NBT_Node::NBT_String, uint64_t>> mslBlock{};
	MapSortList<std::unordered_map<Item, uint64_t, decltype(&Item::Hash), decltype(&Item::Equal)>> mslTileEntityContainer{ .mapItemCounter{128, &Item::Hash, &Item::Equal} };
	//MapSortList<std::map<TileEntityKey, uint64_t>> mslTileEntityContainer{};
	//MapSortList mslEntity{};
	//MapSortList mslEntityContainer{};
};

using RegionStatsList = std::vector<RegionStats>;

RegionStatsList RegionProcess(const NBT_Node::NBT_Compound &cpRegions);

/*
TODO:
����ʵ����Ҫת��Ϊ��Ʒ��ʽ������ת����ֱ����ʾʵ����Ϣ
����©���󳵡����׼ܡ��������Ʒչʾ���

ʵ����������Ϣ���

ʵ����Ʒ�����

��Ʒ��Ҫ����ҩ����ҩˮ����ħ����֮���ˡ��̻�����ȼ���ʽ��������ʽ����֪�ɶ�NBT��Ϣ���и�ʽ�����
���ڿ���Ҫ��Ҫ�Ѵ�˩����ʵ���˩������ͳ��

*/