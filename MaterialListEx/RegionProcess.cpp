#include "RegionProcess.h"

#include "BlockProcess.hpp"
#include "TileEntityProcess.hpp"
#include "NBT_Helper.hpp"

#include <xxhash.h>

static const uint64_t NBT_HASH_SALT = 0x9e3779b97f4a7c15;//generate_random_salt();

uint64_t compute_nbt_hash(const NBT_Node::NBT_Compound &tag)
{
	// 盐值与序列化数据合并
	std::string serialized = NBT_Helper::Serialize(tag) + std::to_string(NBT_HASH_SALT);
	return XXH3_64bits(serialized.data(), serialized.size());
}

RegionStatsList RegionProcess(const NBT_Node::NBT_Compound &cpRegions)
{
	RegionStatsList listRegionStats;
	listRegionStats.reserve(cpRegions.size());//提前扩容
	for (const auto &[RgName, RgVal] : cpRegions)//遍历选区
	{
		RegionStats rgsData{ RgName };
		auto &RgCompound = GetCompound(RgVal);

		//方块处理
		{
			auto &current = rgsData.mslBlock;
			auto vtBlockStats = BlockProcess::GetBlockStats(RgCompound);//获取方块统计列表
			for (const auto &itBlock : vtBlockStats)
			{
				//每个方块转换到物品，并通过map进行统计同类物品
				auto istItemList = BlockProcess::BlockStatsToItemStack(itBlock);
				for (const auto &itItem : istItemList)
				{
					current.mapItemCounter[itItem.sItemName] += itItem.u64Counter;//如果key不存在，则自动创建，且保证value为0
				}
			}

			//执行排序
			current.SortElement();
		}

		//方块实体容器处理
		{
			auto &current = rgsData.mslTileEntityContainer;
			auto vtTEContainerStats = TileEntityProcess::GetTileEntityContainerStats(RgCompound);
			for (const auto &it : vtTEContainerStats)
			{
				auto ret = TileEntityProcess::TileEntityContainerStatsToItemStack(it);
				for (auto &itItem : ret)
				{
					current.mapItemCounter[{std::move(itItem.sItemName),std::move(itItem.cpdItemTag)}] += (uint64_t)(uint8_t)itItem.byteItemCount;//先转换到unsigned，然后再进行扩展
				}

				//执行排序
				current.SortElement();
			}
		}

		//实体处理


		//实体容器处理


		//TODO：实体物品栏处理

		listRegionStats.emplace_back(std::move(rgsData));
	}

	return listRegionStats;
}