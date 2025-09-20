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
	using MyMap = Map;

	//map���ڲ���ͳ�ƣ�����Ҫ�����key�����ң�����key��ʲô����ν
	Map map{};//��������״̬����Ʒӳ��map
	//ͨ��vector����map��Ԫ�����򣨴洢pair���ã�std::reference_wrapper<>
	std::vector<std::reference_wrapper<MapPair>> listSort{};

	//��Ϊ�ڲ���Ϊ���ù�ϵ����ֹ����
	MapSortList(const MapSortList &) = delete;
	MapSortList &operator=(const MapSortList &) = delete;

	//�����ṩ��noexcept�ƶ����죬����vector�����ݹ��̻���п���
	//����MapSortList�洢��map��������vector��ԱʧЧ���Ӷ������쳣
	MapSortList(MapSortList &&) = default;
	MapSortList &operator=(MapSortList &&) = default;

	MapSortList(void) = default;
	MapSortList(Map _map) :map(std::move(_map))
	{}

	static bool SortCmp(MapPair &l, MapPair &r)
	{
		if (l.second == r.second)//�����������°�key����
		{
			return (l.first.operator<=><true>(r.first)) < 0;//����
		}
		else//����val����
		{
			return l.second > r.second;//����
		}
	}

	const auto &GetlistSort(void) const
	{
		return listSort;
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

template <typename Map>
struct MapMSL : public Map
{
public:
	using MapVal = typename Map::mapped_type::MyMap;

	size_t s{};
	typename MapVal::hasher h{};
	typename MapVal::key_equal e{};
public:
	//�̳л��๹��
	using Map::Map;
	//�Զ��幹��
	MapMSL(size_t _s, typename MapVal::hasher _h, typename MapVal::key_equal _e) : s(_s), h(_h), e(_e)
	{}

	typename Map::mapped_type &operator[](typename Map::key_type &&_Keyval)
	{
		return (Map::try_emplace(std::move(_Keyval), MapVal{ s, h, e }).first->second);
	}

	typename Map::mapped_type &operator[](const typename Map::key_type &_Keyval)
	{
		return (Map::try_emplace(_Keyval, MapVal{ s, h, e }).first->second);
	}

	void SortElement(void)
	{
		for (auto &[key, val] : *this)//�������ಢ����ÿ��Ԫ�ص�SortElement
		{
			val.SortElement();
		}
	}

	void Merge(const MapMSL<Map> &src)
	{
		for (const auto &[srcKey, srcVal] : src)//����һ��map�ϲ�����
		{
			this->operator[](srcKey).Merge(srcVal);
		}
	}
};

struct RegionStats
{
	NBT_Type::String sRegionName{};

//������
#define MAPSORTLIST_TYPE(key,val) \
MapSortList<std::unordered_map<key, val, decltype(&key::Hash), decltype(&key::Equal)>>

#define MAPSORTLIST(key,val,size,name) \
MAPSORTLIST_TYPE(key,val) name{ {size, &key::Hash, &key::Equal} }

#define MAPMAPSORTLIST(key,val,size,name) \
MapMSL<std::map<NBT_Type::String, MAPSORTLIST_TYPE(key, val)>> name{size, &key::Hash, &key::Equal}

	//���飨ԭ����ʽ��
	//MAPSORTLIST(BlockInfo, uint64_t, 128, mslBlock);

	//���飨ת������Ʒ��ʽ��
	MAPSORTLIST(NoTagItemInfo, uint64_t, 128, mslBlockItem);
	
	//����ʵ�壨ԭ����ʽ���Ȳ���
	//MAPSORTLIST(TileEntityInfo, uint64_t, 128, mslTileEntity);

	//����ʵ������
	MAPSORTLIST(ItemInfo, uint64_t, 128, mslTileEntityContainer);
	MAPMAPSORTLIST(ItemInfo, uint64_t, 128, mmslParentInfoTEC);

	//ʵ�壨ԭ����ʽ��
	MAPSORTLIST(EntityInfo, uint64_t, 128, mslEntity);

	//ʵ�壨ת������Ʒ��ʽ���Ȳ���
	//MAPSORTLIST(ItemInfo, uint64_t, 128, mslEntityItem);
	//������������ôҪ��ʵ���������ų������Ȼ��ѵ�����ת����������

	//ʵ������
	MAPSORTLIST(ItemInfo, uint64_t, 128, mslEntityContainer);
	MAPMAPSORTLIST(ItemInfo, uint64_t, 128, mmslParentInfoEC);

	//ʵ����Ʒ��
	MAPSORTLIST(ItemInfo, uint64_t, 128, mslEntityInventory);
	MAPMAPSORTLIST(ItemInfo, uint64_t, 128, mmslParentInfoEI);

#undef MAPSORTLIST

	void Merge(const RegionStats &src)
	{
		//mslBlock.Merge(src.mslBlock);
		mslBlockItem.Merge(src.mslBlockItem);
		//TileEntityInfo.Merge(src.TileEntityInfo);
		mslTileEntityContainer.Merge(src.mslTileEntityContainer);
		mmslParentInfoTEC.Merge(src.mmslParentInfoTEC);
		mslEntity.Merge(src.mslEntity);
		//mslEntityItem.Merge(src.mslEntityItem);
		mslEntityContainer.Merge(src.mslEntityContainer);
		mmslParentInfoEC.Merge(src.mmslParentInfoEC);
		mslEntityInventory.Merge(src.mslEntityInventory);
		mmslParentInfoEI.Merge(src.mmslParentInfoEI);
	}

	void SortElement(void)
	{
		//mslBlock.SortElement();
		mslBlockItem.SortElement();
		//mslTileEntity.SortElement();		
		mslTileEntityContainer.SortElement();
		mmslParentInfoTEC.SortElement();
		mslEntity.SortElement();
		//mslEntityItem.SortElement();		
		mslEntityContainer.SortElement();
		mmslParentInfoEC.SortElement();
		mslEntityInventory.SortElement();
		mmslParentInfoEI.SortElement();
	}
};

using RegionStatsList = std::vector<RegionStats>;

RegionStatsList RegionProcess(const NBT_Type::Compound &cpRegions);

//�ϲ�����ѡ��
RegionStats MergeRegionStatsList(const RegionStatsList &listRegionStats);