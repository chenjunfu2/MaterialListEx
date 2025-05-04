#pragma once
#include "NBT_Node.hpp"

struct RegionStats
{
	const NBT_Node::NBT_String *psRegionName{};

	template<typename tMap>
	struct MapSortList//Ҫ��tValue����Ҫ��ʹ��==��>
	{
		tMap mapItemCounter;//��������״̬����Ʒӳ��map
		using MapPair = decltype(mapItemCounter)::value_type;
		std::vector<std::reference_wrapper<MapPair>> vecSortItem{};//ͨ��vector����map��Ԫ�����򣨴洢pair���ã�std::reference_wrapper<>

		static bool SortCmp(MapPair &l, MapPair &r)
		{
			if (l.second == r.second)//�����������°�key���ֵ���
			{
				return l.first < r.first;//����
			}
			else//������ֵ��С
			{
				return l.second > r.second;//����
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