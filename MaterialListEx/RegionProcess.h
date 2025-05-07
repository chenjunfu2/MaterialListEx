#pragma once

#include <algorithm>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>

#include <xxhash.h>

#include "NBT_Node.hpp"
#include "NBT_Helper.hpp"


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

struct TileEntityKey
{
	NBT_Node::NBT_String sName{};
	NBT_Node::NBT_Compound cpdTag{};
	//XXH64_hash_t cpdHash{ HashNbtTag(cpdTag) };//初始化顺序严格按照声明顺序，此处无问题
	
	//XXH64_hash_t HashNbtTag(const NBT_Node::NBT_Compound &tag)
	//{
	//	constexpr static XXH64_hash_t NBT_HASH_SEED = 0xDE35B92A7F41706C;
	//	return NBT_Helper::Hash(tag, NBT_HASH_SEED);
	//}

	//static size_t Hash(const TileEntityKey &self)
	//{
	//	return std::hash<XXH64_hash_t>{}(self.cpdHash);
	//}
	//
	//static bool Equal(const TileEntityKey &_l, const TileEntityKey &_r)
	//{
	//	return _l.sName == _r.sName && _l.cpdHash == _r.cpdHash && _l.cpdTag == _r.cpdTag;
	//}

	inline std::weak_ordering operator<=>(const TileEntityKey &_r) const
	{
		//先按照名称排序
		if (auto tmp = (sName <=> _r.sName); tmp != 0)
		{
			return tmp;
		}

		//然后按照哈希序
		//if (auto tmp = (cpdHash <=> _r.cpdHash); tmp != 0)
		//{
		//	return tmp;
		//}

		//都相同最后按照Tag序
		return cpdTag <=> _r.cpdTag;
	}
};


struct RegionStats
{
	NBT_Node::NBT_String sRegionName{};
	//MapSortList<std::unordered_map<NBT_Node::NBT_String, uint64_t>> mslBlock{};
	MapSortList<std::map<NBT_Node::NBT_String, uint64_t>> mslBlock{};
	//MapSortList<std::unordered_map<TileEntityKey, uint64_t, decltype(&TileEntityKey::Hash), decltype(&TileEntityKey::Equal)>> mslTileEntityContainer{ {64 ,&TileEntityKey::Hash, &TileEntityKey::Equal} };
	MapSortList<std::map<TileEntityKey, uint64_t>> mslTileEntityContainer{};
	//MapSortList mslEntity{};
	//MapSortList mslEntityContainer{};
};

using RegionStatsList = std::vector<RegionStats>;

RegionStatsList RegionProcess(const NBT_Node::NBT_Compound &cpRegions);