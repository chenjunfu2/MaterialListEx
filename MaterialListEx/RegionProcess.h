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
部分实体需要转换为物品形式，不能转换的直接显示实体信息
比如漏斗矿车、盔甲架、掉落物、物品展示框等

实体内容器信息解包

实体物品栏解包

物品需要解析药箭、药水、附魔、谜之炖菜、烟花火箭等级样式，旗帜样式等已知可读NBT信息进行格式化输出
还在考虑要不要把带拴绳的实体的拴绳进行统计

*/