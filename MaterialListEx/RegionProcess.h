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
	//XXH64_hash_t cpdHash{ HashNbtTag(cpdTag) };//��ʼ��˳���ϸ�������˳�򣬴˴�������
	
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
		//�Ȱ�����������
		if (auto tmp = (sName <=> _r.sName); tmp != 0)
		{
			return tmp;
		}

		//Ȼ���չ�ϣ��
		//if (auto tmp = (cpdHash <=> _r.cpdHash); tmp != 0)
		//{
		//	return tmp;
		//}

		//����ͬ�����Tag��
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