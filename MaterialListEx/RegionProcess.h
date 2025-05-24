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
	MapSortList<std::unordered_map<Item, uint64_t, decltype(&Item::Hash), decltype(&Item::Equal)>> mslTileEntityContainer{ .mapItemCounter{128, &Item::Hash, &Item::Equal} };
	MapSortList<std::unordered_map<Item, uint64_t, decltype(&Item::Hash), decltype(&Item::Equal)>> mslEntity{ .mapItemCounter{128, &Item::Hash, &Item::Equal} };
	MapSortList<std::unordered_map<Item, uint64_t, decltype(&Item::Hash), decltype(&Item::Equal)>> mslEntityContainer{ .mapItemCounter{128, &Item::Hash, &Item::Equal} };
	MapSortList<std::unordered_map<Item, uint64_t, decltype(&Item::Hash), decltype(&Item::Equal)>> mslEntityInventory{ .mapItemCounter{128, &Item::Hash, &Item::Equal} };
};

using RegionStatsList = std::vector<RegionStats>;

RegionStatsList RegionProcess(const NBT_Node::NBT_Compound &cpRegions);