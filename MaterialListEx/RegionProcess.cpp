#include "RegionProcess.h"

#include "BlockProcess.hpp"
#include "TileEntityProcess.hpp"
#include "EntityProcess.hpp"

RegionStatsList RegionProcess(const NBT_Node::NBT_Compound &cpRegions)
{
	RegionStatsList listRegionStats;
	listRegionStats.reserve(cpRegions.size());//提前扩容
	for (const auto &[RgName, RgVal] : cpRegions)//遍历选区
	{
		RegionStats rgsData{ RgName };
		auto &RgCompound = GetCompound(RgVal);

		//方块到物品处理
		{
			auto &current = rgsData.mslBlockItem;
			auto listBlockStats = BlockProcess::GetBlockStats(RgCompound);//获取方块统计列表
			for (const auto &itBlock : listBlockStats)
			{
				//每个方块转换到物品，并通过map进行统计同类物品
				auto istItemList = BlockProcess::BlockStatsToItemStack(itBlock);
				for (const auto &itItem : istItemList)
				{
					Item tmp{ std::move(itItem.sItemName) };//转移所有权
					current.mapItemCounter[std::move(tmp)] += itItem.u64Counter;//如果key不存在，则自动创建，且保证value为0
				}
			}

			//执行排序
			current.SortElement();
		}

		//方块实体容器处理
		{
			auto &current = rgsData.mslTileEntityContainer;
			auto listTEContainerStats = TileEntityProcess::GetTileEntityContainerStats(RgCompound);
			for (const auto &it : listTEContainerStats)
			{
				//转换每个方块实体
				auto ret = TileEntityProcess::TileEntityContainerStatsToItemStack(it);
				for (auto &itItem : ret)
				{
					Item tmp{ std::move(itItem.sItemName), std::move(itItem.cpdItemTag) };//转移所有权
					current.mapItemCounter[std::move(tmp)] += (uint64_t)(uint8_t)itItem.byteItemCount;//先转换到unsigned，然后再进行扩展
				}
			}

			//执行排序
			current.SortElement();
		}

		//实体处理
		{
			//与上面有些区别，在读取出来后需要做三步
			//第一步先解出实体本身，第二步解出容器，第三步再解出物品栏
			//因为你麻将方块和方块实体分开存放，而实体和实体tag是在一起的，
			//分开处理反而很麻烦，只能这样（麻将神操作能不喷的都是神人了）
			//注意实体非常特殊，很多时候获取方式并不局限于从物品得来，
			//没办法直接转换到物品，所以单独放一个实体表
			auto &current = rgsData.mslEntity;
			auto &curContainer = rgsData.mslEntityContainer;
			auto &curInventory = rgsData.mslEntityInventory;

			auto listEntityStats = EntityProcess::GetEntityStats(RgCompound);
			for (const auto &it : listEntityStats)
			{


			}



			//执行排序
			current.SortElement();
			curContainer.SortElement();
			curInventory.SortElement();
		}

		//上面几个没有带块语句的无名块，用于控制变量作用域，提升代码可读性

		//处理完每个region后放入region列表
		listRegionStats.emplace_back(std::move(rgsData));
	}

	return listRegionStats;
}