#pragma once
#include "NBT_Node.hpp"

struct RegionStats
{
	const NBT_Node::NBT_String *psRegionName{};

	template<typename tMap>
	struct MapSortList//要求tValue必须要能使用==和>
	{
		tMap mapItemCounter;//创建方块状态到物品映射map
		using MapPair = decltype(mapItemCounter)::value_type;
		std::vector<std::reference_wrapper<MapPair>> vecSortItem{};//通过vector创建map的元素排序（存储pair引用）std::reference_wrapper<>

		static bool SortCmp(MapPair &l, MapPair &r)
		{
			if (l.second == r.second)//数量相等情况下按key的字典序
			{
				return l.first < r.first;//升序
			}
			else//否则按数值大小
			{
				return l.second > r.second;//降序
			}
		}
	};

	struct TileEntityKey
	{
		NBT_Node::NBT_String sName{};
		NBT_Node::NBT_Compound cpdTag{};

		//inline std::strong_ordering operator<=>(const TileEntityKey &_r) const
		//{
		//	if (auto tmp = (sName <=> _r.sName); tmp != 0)
		//	{
		//		return tmp;
		//	}
		//
		//	return ;
		//}

		//inline TileEntityVal &Add(uint64_t _u64Count, NBT_Node::NBT_Compound cpdTag)
		//{
		//	return;
		//}
	};

	MapSortList<std::map<NBT_Node::NBT_String, uint64_t>> mslBlock;
	MapSortList<std::map<TileEntityKey, uint64_t>> mslTileEntityContainer;
	//MapSortList mslEntity;
	//MapSortList mslEntityContainer;
};

using RegionStatsList = std::vector<RegionStats>;

RegionStatsList RegionProcess(const NBT_Node::NBT_Compound &cpRegions);