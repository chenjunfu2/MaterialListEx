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

	//map用于查重统计，不需要真的用key来查找，所以key是什么无所谓
	Map mapItemCounter{};//创建方块状态到物品映射map
	//通过vector创建map的元素排序（存储pair引用）std::reference_wrapper<>
	std::vector<std::reference_wrapper<MapPair>> vecSortItem{};

	static bool SortCmp(MapPair &l, MapPair &r)
	{
		if (l.second == r.second)//数量相等情况下按key排序
		{
			return l.first < r.first;//升序
		}
		else//否则val排序
		{
			return l.second > r.second;//降序
		}
	}
	
	void SortElement(void)
	{
		//提前扩容减少插入开销
		vecSortItem.reserve(mapItemCounter.size());
		vecSortItem.assign(mapItemCounter.begin(), mapItemCounter.end());//迭代器范围插入
		//对物品按数量进行排序
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