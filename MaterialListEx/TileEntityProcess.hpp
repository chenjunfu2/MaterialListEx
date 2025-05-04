#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"

#include <unordered_map>

class TileEntityProcess
{
public:
	TileEntityProcess() = delete;
	~TileEntityProcess() = delete;


	struct TileEntityContainerStats
	{
		const NBT_Node::NBT_String *psTileEntityName{};
		struct
		{
			const NBT_Node *pItems{};
			NBT_Node::NBT_TAG enType{ NBT_Node::TAG_End };
		};
	};

private:
	//处理多种集合数据情况并映射到TileEntityContainerStats
	static bool MapTileEntityContainerStats(const NBT_Node::NBT_Compound &teCompound, TileEntityContainerStats &teStats)
	{
		/*
			有好几种情况：item-Compound、Items-List、特殊名称-Compound
		*/

		//尝试寻找Items（普通多格容器）
		if (const auto pItems = teCompound.Search(MU8STR("Items"));
			pItems != NULL && pItems->IsList())
		{
			teStats.pItems = pItems;
			teStats.enType = NBT_Node::TAG_List;
			return true;
		}

		//尝试处理特殊方块
		struct DataInfo
		{
			NBT_Node::NBT_String sTagName{};
			NBT_Node::NBT_TAG enType{ NBT_Node::TAG_End };
		};

		//用于映射特殊的方块实体容器里物品的名字
		const static std::unordered_map<NBT_Node::NBT_String, DataInfo> mapContainerItemName =
		{
			{MU8STR("minecraft:jukebox"),{MU8STR("RecordItem"),NBT_Node::TAG_Compound}},
			{MU8STR("minecraft:lectern"),{MU8STR("Book"),NBT_Node::TAG_Compound}},
			{MU8STR("minecraft:brushable_block"),{MU8STR("item"),NBT_Node::TAG_Compound}},
		};

		if (teStats.psTileEntityName == NULL)
		{
			return false;
		}

		//查找方块实体id是否在map中
		const auto findIt = mapContainerItemName.find(*teStats.psTileEntityName);
		if (findIt == mapContainerItemName.end())//不在，不是可以存放物品的容器，跳过
		{
			return false;//跳过此方块实体
		}

		//通过映射名查找对应物品存储位置
		const auto pSearch = teCompound.Search(findIt->second.sTagName);
		if (pSearch == NULL)//查找失败
		{
			return false;//跳过此方块实体
		}

		//放入结构内
		teStats.pItems = pSearch;
		teStats.enType = findIt->second.enType;
		return true;
	}

public:
	static std::vector<TileEntityContainerStats> GetTileEntityContainerStats(const NBT_Node::NBT_Compound &RgCompound)
	{
		//获取方块实体列表
		const auto &listTileEntity = RgCompound.GetList(MU8STR("TileEntities"));
		std::vector<TileEntityContainerStats> vtTileEntityStats{};
		vtTileEntityStats.reserve(listTileEntity.size());//提前扩容

		for (const auto &it : listTileEntity)
		{
			const auto &cur = GetCompound(it);

			TileEntityContainerStats teStats{ &cur.GetString(MU8STR("id")) };
			if (MapTileEntityContainerStats(cur, teStats))
			{
				vtTileEntityStats.emplace_back(std::move(teStats));
			}
		}

		return vtTileEntityStats;
	}//func

	struct ItemStack
	{
		NBT_Node::NBT_String sItemName{};//物品名
		NBT_Node::NBT_Compound cpdItemTag{};//物品标签
		NBT_Node::NBT_Byte byteItemCount = 0;//物品计数器
	};

	using ItemStackList = std::vector<ItemStack>;

private:
	static void AddItems(const NBT_Node::NBT_Compound &cpdItem, ItemStackList &vtItemStackList, size_t szStackDepth)
	{
		if (szStackDepth <= 0)//递归栈深度检查
		{
			return;//不处理直接返回
		}

		//先插入
		vtItemStackList.emplace_back
		(
			cpdItem.GetString(MU8STR("id")),
			NBT_Node::NBT_Compound{},//此处先留空，防止深度嵌套容器的大量拷贝
			cpdItem.GetByte("Count")
		);

		//如果pcpdTag为NULL则跳过处理直接返回
		const auto pcpdTag = cpdItem.Search(MU8STR("tag"));
		if (pcpdTag == NULL)
		{
			return;
		}

		//然后获取尾部数据为当前数据
		auto &current = vtItemStackList.back();//因为有可能进行写入，需要设置为非const
		const auto &cpdTag = pcpdTag->GetCompound();//先引用，然后处理容器，如果是容器则不拷贝，否则拷贝Tag以便处理特殊物品名（药水箭、药水、附魔书等名称随nbt变化的物品）

		//检查物品是否有方块实体tag（C你*的麻将，明明方块实体叫TileEntity，怎么这里又叫BlockEntity，真的无语了）
		//（前面写方块状态转换就麻烦的要死，各种不统一，一个含水都能搞出几种情况，写到这里真的蚌埠住了，我喷死你*的麻将）
		const auto pBlockEntityTag = cpdTag.Search(MU8STR("BlockEntityTag"));
		if (pBlockEntityTag != NULL)//处理方块实体
		{
			const auto &cpdBlockEntityTag = pBlockEntityTag->GetCompound();

			//映射需要物品id，获取一下（绝了，甚至还存在有BlockEntityTag内没有物品id的）
			const auto pId = cpdBlockEntityTag.Search(MU8STR("id"));
			TileEntityContainerStats teStats{ (pId != NULL) ? &pId->GetString() : &current.sItemName };//没有id的情况下直接使用物品名

			//处理BlockEntityTag，转换到容器物品统计信息
			if (MapTileEntityContainerStats(cpdBlockEntityTag, teStats))//尝试映射
			{
				//成功后递归解包内容
				_TileEntityContainerStatsToItemStack(teStats, vtItemStackList, szStackDepth);//循环递归
			}
		}
		//（卧槽收纳袋还不是BlockEntityTag内部嵌套Items的，是直接Items的，还得特殊处理，没救了麻将，我服了）
		else if (current.sItemName == MU8STR("minecraft:bundle"))//没有BlockEntityTag，尝试处理收纳袋
		{
			const auto pItems = cpdTag.Search(MU8STR("Items"));
			TileEntityContainerStats teStats{ NULL,pItems,NBT_Node::TAG_List };//递归处理不需要id，且会处理pItems可能为NULL的情况
			_TileEntityContainerStatsToItemStack(teStats, vtItemStackList, szStackDepth);//循环递归
		}
		else//如果上面都没处理，则属于非容器，拷贝Tag以便后续分析，容器则跳过拷贝，Tag留空
		{
			current.cpdItemTag = cpdTag;//拷贝
		}
	};

	static void _TileEntityContainerStatsToItemStack(const TileEntityContainerStats &stContainerStats, ItemStackList &vtItemStackList, size_t szStackDepth)
	{
		if (stContainerStats.pItems == NULL)
		{
			return;
		}

		//读取每个物品的集合并解包出内容插入到ItemStack

		if (stContainerStats.enType == NBT_Node::TAG_Compound)//只有一格物品
		{
			AddItems(stContainerStats.pItems->GetCompound(), vtItemStackList, szStackDepth - 1);
		}
		else if (stContainerStats.enType == NBT_Node::TAG_List)//多格物品列表
		{
			const auto tmp = stContainerStats.pItems->GetList();
			for (const auto &it : tmp)
			{
				AddItems(it.GetCompound(), vtItemStackList, szStackDepth - 1);
			}
		}
		//else {}
	}

public:
	static ItemStackList TileEntityContainerStatsToItemStack(const TileEntityContainerStats &stContainerStats, size_t szStackDepth = 64)
	{
		ItemStackList ret;
		_TileEntityContainerStatsToItemStack(stContainerStats, ret, szStackDepth);
		return ret;
	}
};