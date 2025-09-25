#include "RegionProcess.h"

//合并所有选区
RegionStats MergeRegionStatsList(const RegionStatsList &listRegionStats)
{
	RegionStats allRegionStats{};
	for (const auto &it : listRegionStats)//遍历所有选区合并到allRegionStats内
	{
		allRegionStats.Merge(it);
	}

	//全部排序
	allRegionStats.SortElement();
	
	return allRegionStats;
}

RegionStatsList RegionProcess(const NBT_Type::Compound &cpRegions)
{
	RegionStatsList listRegionStats;
	listRegionStats.reserve(cpRegions.Size());//提前扩容
	for (const auto &[RgName, RgVal] : cpRegions)//遍历选区
	{
		RegionStats rgsData{ RgName };
		auto &RgCompound = GetCompound(RgVal);

		//方块到物品处理
		{
			//auto &current = rgsData.mslBlock;
			auto &curBlockItem = rgsData.mslBlockItem;
			auto listBlockStats = BlockProcess::GetBlockStats(RgCompound);//获取方块统计列表
			for (const auto &itBlock : listBlockStats)
			{
				//转换方块
				//auto tmpBlock = BlockProcess::BlockStatsToBlockInfo(itBlock);
				//current.map[std::move(tmpBlock)] += itBlock.u64Counter;//艸，这里要加计数器，而不是加一，因为前面都统计完了

				//每个方块转换到物品，并通过map进行统计同类物品
				auto tmp = BlockProcess::BlockStatsToItemStack(itBlock);
				for (auto &itItem : tmp)
				{
					curBlockItem.map[{ std::move(itItem.sItemName) }] += itItem.u64Counter;//如果key不存在，则自动创建，且保证value为0
				}
			}

			//执行排序
			//current.SortElement();
			curBlockItem.SortElement();
		}

		//方块实体容器处理
		{
			auto &current = rgsData.mslTileEntityContainer;
			auto &curInfoTEC = rgsData.mmslParentInfoTEC;

			auto listTEContainerStats = TileEntityProcess::GetTileEntityContainerStats(RgCompound);
			for (const auto &it : listTEContainerStats)//处理每个方块实体
			{
				//转换方块实体
				auto tmp = TileEntityProcess::TileEntityContainerStatsToItemStack(it);
				tmp = ItemProcess::ItemStackListUnpackContainer(std::move(tmp));//解包物品内部的容器

				//获取容器名称
				auto sParentName = it.psTileEntityName == NULL ? NBT_Type::String{} : *it.psTileEntityName;
				auto &mpInfoTEC = curInfoTEC[sParentName];

				//遍历所有转换后的物品并合并相同
				for (auto &itItem : tmp)
				{
					mpInfoTEC.map[{ itItem.sItemName, itItem.cpdItemTag }] += itItem.u64ItemCount;
					current.map[{ itItem.sItemName, itItem.cpdItemTag }] += itItem.u64ItemCount;
				}
			}

			//执行排序
			current.SortElement();
			curInfoTEC.SortElement();
		}

		//实体处理
		{
			//与上面有些区别，在读取出来后需要做三步
			//第一步先解出实体本身，第二步解出容器，第三步再解出物品栏
			//因为你麻将方块和方块实体分开存放，而实体和实体tag是在一起的，
			//分开处理反而很麻烦，只能这样（麻将神操作能不喷的都是神人了）
			//注意实体非常特殊，很多时候获取方式并不局限于从物品得来，
			//没办法直接转换到物品，所以单独放一个实体表
			//实体转换物品还是有必要的，比如船，是什么材质由实体nbt决定，而实体名都叫boat
			auto &current = rgsData.mslEntity;
			auto &curContainer = rgsData.mslEntityContainer;
			auto &curInventory = rgsData.mslEntityInventory;
			auto &curInfoEC = rgsData.mmslParentInfoEC;
			auto &curInfoEI = rgsData.mmslParentInfoEI;

			auto listEntityStats = EntityProcess::GetEntityStats(RgCompound);
			for (const auto &it : listEntityStats)
			{
				//转换实体
				auto tmpEntity = EntityProcess::EntityStatsToEntityInfo(it);
				current.map[std::move(tmpEntity)] += 1;//每次遇到+1即可，因为每次处理的实体只有一个

				//转换实体物品栏和容器
				auto tmpSlot = EntityProcess::EntityStatsToEntitySlot(it);

				//对内部的物品容器进行解包
				tmpSlot.listContainer = ItemProcess::ItemStackListUnpackContainer(std::move(tmpSlot.listContainer));
				tmpSlot.listInventory = ItemProcess::ItemStackListUnpackContainer(std::move(tmpSlot.listInventory));

				//获取实体名称并获取带容器名物品信息映射
				auto sParentName = it.psEntityName == NULL ? NBT_Type::String{} : *it.psEntityName;
				auto &mpInfoEC = curInfoEC[sParentName];
				auto &mpInfoEI = curInfoEI[sParentName];

				//遍历并合并相同物品
				for (auto &itItem : tmpSlot.listContainer)
				{
					mpInfoEC.map[{ itItem.sItemName, itItem.cpdItemTag }] += itItem.u64ItemCount;
					curContainer.map[{ itItem.sItemName, itItem.cpdItemTag }] += itItem.u64ItemCount;
				}
				for (auto &itItem : tmpSlot.listInventory)
				{
					mpInfoEI.map[{ itItem.sItemName, itItem.cpdItemTag }] += itItem.u64ItemCount;
					curInventory.map[{ itItem.sItemName, itItem.cpdItemTag }] += itItem.u64ItemCount;
				}
			}

			//执行排序
			current.SortElement();
			curContainer.SortElement();
			curInventory.SortElement();
			curInfoEC.SortElement();
			curInfoEI.SortElement();
		}

		//上面几个没有带块语句的无名块，用于控制变量作用域，提升代码可读性

		//处理完每个region后放入region列表
		listRegionStats.emplace_back(std::move(rgsData));
	}

	return listRegionStats;
}