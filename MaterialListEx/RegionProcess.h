#pragma once

#include <algorithm>
#include <vector>
#include <map>
#include <functional>

#include "NBT_Node.hpp"


template<typename tKey, typename tVal>
struct MapSortList
{
	using Map = std::map<tKey, tVal>;
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

struct TileEntityKey
{
	NBT_Node::NBT_String sName{};
	NBT_Node::NBT_Compound cpdTag{};

	inline std::weak_ordering operator<=>(const TileEntityKey &_r) const
	{
		if (auto tmp = (sName <=> _r.sName); tmp != 0)
		{
			return tmp;
		}

		return cpdTag <=> _r.cpdTag;
	}
};


struct RegionStats
{
	NBT_Node::NBT_String sRegionName{};
	MapSortList<NBT_Node::NBT_String, uint64_t> mslBlock{};
	MapSortList<TileEntityKey, uint64_t> mslTileEntityContainer{};
	//MapSortList mslEntity{};
	//MapSortList mslEntityContainer{};
};

using RegionStatsList = std::vector<RegionStats>;

RegionStatsList RegionProcess(const NBT_Node::NBT_Compound &cpRegions);