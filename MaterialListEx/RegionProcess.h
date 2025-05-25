#pragma once

#include <algorithm>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>

#include "NBT_Node.hpp"
#include "ItemProcess.hpp"
#include "BlockProcess.hpp"
#include "TileEntityProcess.hpp"
#include "EntityProcess.hpp"

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

	//���飨ԭ����ʽ���Ȳ���
	//MapSortList<std::unordered_map<Block, uint64_t, decltype(&Block::Hash), decltype(&Block::Equal)>> mslBlock{ .mapItemCounter{128, &Block::Hash, &Block::Equal} };

	//���飨ת������Ʒ��ʽ��
	MapSortList<std::unordered_map<ItemInfo, uint64_t, decltype(&ItemInfo::Hash), decltype(&ItemInfo::Equal)>> mslBlockItem{ .mapItemCounter{128, &ItemInfo::Hash, &ItemInfo::Equal} };
	
	//����ʵ�壨ԭ����ʽ���Ȳ���
	//MapSortList<std::unordered_map<TileEntity, uint64_t, decltype(&TileEntity::Hash), decltype(&TileEntity::Equal)>> mslTileEntity{ .mapItemCounter{128, &TileEntity::Hash, &TileEntity::Equal} };

	//����ʵ������
	MapSortList<std::unordered_map<ItemInfo, uint64_t, decltype(&ItemInfo::Hash), decltype(&ItemInfo::Equal)>> mslTileEntityContainer{ .mapItemCounter{128, &ItemInfo::Hash, &ItemInfo::Equal} };

	//ʵ�壨ԭ����ʽ��
	MapSortList<std::unordered_map<Entity, uint64_t, decltype(&Entity::Hash), decltype(&Entity::Equal)>> mslEntity{ .mapItemCounter{128, &Entity::Hash, &Entity::Equal} };

	//ʵ�壨ת������Ʒ��ʽ���Ȳ���
	//MapSortList<std::unordered_map<ItemInfo, uint64_t, decltype(&ItemInfo::Hash), decltype(&ItemInfo::Equal)>> mslEntityItem{ .mapItemCounter{128, &ItemInfo::Hash, &ItemInfo::Equal} };

	//ʵ������
	MapSortList<std::unordered_map<ItemInfo, uint64_t, decltype(&ItemInfo::Hash), decltype(&ItemInfo::Equal)>> mslEntityContainer{ .mapItemCounter{128, &ItemInfo::Hash, &ItemInfo::Equal} };
	//ʵ����Ʒ��
	MapSortList<std::unordered_map<ItemInfo, uint64_t, decltype(&ItemInfo::Hash), decltype(&ItemInfo::Equal)>> mslEntityInventory{ .mapItemCounter{128, &ItemInfo::Hash, &ItemInfo::Equal} };
};

using RegionStatsList = std::vector<RegionStats>;

RegionStatsList RegionProcess(const NBT_Node::NBT_Compound &cpRegions);