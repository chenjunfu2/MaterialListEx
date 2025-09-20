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

	//map用于查重统计，不需要真的用key来查找，所以key是什么无所谓
	Map map{};//创建方块状态到物品映射map
	//通过vector创建map的元素排序（存储pair引用）std::reference_wrapper<>
	std::vector<std::reference_wrapper<MapPair>> listSort{};

	//因为内部互为引用关系，禁止拷贝
	MapSortList(const MapSortList &) = delete;
	MapSortList &operator=(const MapSortList &) = delete;

	//必须提供此noexcept移动构造，否则vector在扩容过程会进行拷贝
	//导致MapSortList存储的map迭代器的vector成员失效，从而引发异常
	MapSortList(MapSortList &&) = default;
	MapSortList &operator=(MapSortList &&) = default;

	MapSortList(void) = default;
	MapSortList(Map _map) :map(std::move(_map))
	{}

	static bool SortCmp(MapPair &l, MapPair &r)
	{
		if (l.second == r.second)//数量相等情况下按key排序
		{
			return (l.first.operator<=><true>(r.first)) < 0;//升序
		}
		else//否则val排序
		{
			return l.second > r.second;//降序
		}
	}

	const auto &GetlistSort(void) const
	{
		return listSort;
	}
	
	void SortElement(void)
	{
		//提前扩容减少插入开销
		listSort.reserve(map.size());
		listSort.assign(map.begin(), map.end());//迭代器范围插入
		//对物品按数量进行排序
		std::sort(listSort.begin(), listSort.end(), SortCmp);
	}

	void Merge(const MapSortList<Map> &src)//合并
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
	//继承基类构造
	using Map::Map;
	//自定义构造
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
		for (auto &[key, val] : *this)//遍历基类并调用每个元素的SortElement
		{
			val.SortElement();
		}
	}

	void Merge(const MapMSL<Map> &src)
	{
		for (const auto &[srcKey, srcVal] : src)//把另一个map合并进来
		{
			this->operator[](srcKey).Merge(srcVal);
		}
	}
};

struct RegionStats
{
	NBT_Type::String sRegionName{};

//简化声明
#define MAPSORTLIST_TYPE(key,val) \
MapSortList<std::unordered_map<key, val, decltype(&key::Hash), decltype(&key::Equal)>>

#define MAPSORTLIST(key,val,size,name) \
MAPSORTLIST_TYPE(key,val) name{ {size, &key::Hash, &key::Equal} }

#define MAPMAPSORTLIST(key,val,size,name) \
MapMSL<std::map<NBT_Type::String, MAPSORTLIST_TYPE(key, val)>> name{size, &key::Hash, &key::Equal}

	//方块（原本形式）
	//MAPSORTLIST(BlockInfo, uint64_t, 128, mslBlock);

	//方块（转换到物品形式）
	MAPSORTLIST(NoTagItemInfo, uint64_t, 128, mslBlockItem);
	
	//方块实体（原本形式）先不做
	//MAPSORTLIST(TileEntityInfo, uint64_t, 128, mslTileEntity);

	//方块实体容器
	MAPSORTLIST(ItemInfo, uint64_t, 128, mslTileEntityContainer);
	MAPMAPSORTLIST(ItemInfo, uint64_t, 128, mmslParentInfoTEC);

	//实体（原本形式）
	MAPSORTLIST(EntityInfo, uint64_t, 128, mslEntity);

	//实体（转换到物品形式）先不做
	//MAPSORTLIST(ItemInfo, uint64_t, 128, mslEntityItem);
	//如果做这个，那么要在实体容器内排除掉落物，然后把掉落物转换到这里面

	//实体容器
	MAPSORTLIST(ItemInfo, uint64_t, 128, mslEntityContainer);
	MAPMAPSORTLIST(ItemInfo, uint64_t, 128, mmslParentInfoEC);

	//实体物品栏
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

//合并所有选区
RegionStats MergeRegionStatsList(const RegionStatsList &listRegionStats);