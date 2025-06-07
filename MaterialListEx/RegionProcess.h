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
	Map map{};//��������״̬����Ʒӳ��map
	//ͨ��vector����map��Ԫ�����򣨴洢pair���ã�std::reference_wrapper<>
	std::vector<std::reference_wrapper<MapPair>> listSort{};

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
		listSort.reserve(map.size());
		listSort.assign(map.begin(), map.end());//��������Χ����
		//����Ʒ��������������
		std::sort(listSort.begin(), listSort.end(), SortCmp);
	}

	void Merge(const MapSortList<Map> &src)//�ϲ�
	{
		for (const auto &[srcKey, srcVal] : src.map)
		{
			map[srcKey] += srcVal;
		}
	}
};

struct RegionStats
{
	NBT_Node::NBT_String sRegionName{};

	RegionStats(void) = default;
	RegionStats(NBT_Node::NBT_String _sRegionName) :sRegionName(_sRegionName)
	{}
	
	RegionStats &operator=(const RegionStats &) = default;
	RegionStats &operator=(RegionStats &&) noexcept = default;
	
	//�����ṩ��noexcept�ƶ����죬����vector�����ݹ��̻���п���
	//��������Ǿ�ȻMapSortList�����ṩ��ͬΪ�ۺ����͵�RegionStats��Ҫ�ṩ��
	//����MapSortList�洢��map��������vector��ԱʧЧ���Ӷ������쳣
	RegionStats(RegionStats && _Right) noexcept = default;
	RegionStats(const RegionStats & _Right) = default;

	//������
#define MAPSORTLIST(key,val,size,name) \
MapSortList<std::unordered_map<key, val, decltype(&key::Hash), decltype(&key::Equal)>> name{ .map{size, &key::Hash, &key::Equal} }

	//���飨ԭ����ʽ��
	MAPSORTLIST(BlockInfo, uint64_t, 128, mslBlock);

	//���飨ת������Ʒ��ʽ��
	MAPSORTLIST(NoTagItemInfo, uint64_t, 128, mslBlockItem);
	
	//����ʵ�壨ԭ����ʽ���Ȳ���
	//MAPSORTLIST(TileEntityInfo, uint64_t, 128, mslTileEntity);

	//����ʵ������
	MAPSORTLIST(ItemInfo, uint64_t, 128, mslTileEntityContainer);

	//ʵ�壨ԭ����ʽ��
	MAPSORTLIST(EntityInfo, uint64_t, 128, mslEntity);

	//ʵ�壨ת������Ʒ��ʽ���Ȳ���
	//MAPSORTLIST(ItemInfo, uint64_t, 128, mslEntityItem);
	//������������ôҪ��ʵ���������ų������Ȼ��ѵ�����ת����������

	//ʵ������
	MAPSORTLIST(ItemInfo, uint64_t, 128, mslEntityContainer);

	//ʵ����Ʒ��
	MAPSORTLIST(ItemInfo, uint64_t, 128, mslEntityInventory);

#undef MAPSORTLIST
};

using RegionStatsList = std::vector<RegionStats>;

RegionStatsList RegionProcess(const NBT_Node::NBT_Compound &cpRegions);

//�ϲ�����ѡ��
RegionStats MergeRegionStatsList(const RegionStatsList &listRegionStats);