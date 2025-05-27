#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"
#include "ItemProcess.hpp"

#include <compare>

struct EntityInfo
{
	NBT_Node::NBT_String sName{};
	//除了名字外，剩下全丢了，没意义
	uint64_t u64Hash{ DataHash() };

public:
	uint64_t DataHash(void)
	{
		static_assert(std::is_same_v<XXH64_hash_t, uint64_t>, "Hash type does not match the required type.");

		constexpr static XXH64_hash_t HASH_SEED = 0x83B0'1A83'062C'4F5D;

		return XXH64(sName.data(), sName.size(), HASH_SEED);
	}

public:
	static size_t Hash(const EntityInfo &self)
	{
		return self.u64Hash;
	}

	static bool Equal(const EntityInfo &_l, const EntityInfo &_r)
	{
		return _l.u64Hash == _r.u64Hash && _l.sName == _r.sName;
	}

	inline std::strong_ordering operator<=>(const EntityInfo &_r) const
	{
		//先按照哈希序
		if (auto tmp = (u64Hash <=> _r.u64Hash); tmp != 0)
		{
			return tmp;
		}

		//后按照名称序
		return sName <=> _r.sName;
	}
};

class EntityProcess
{
public:
	EntityProcess() = delete;
	~EntityProcess() = delete;

	//5~6为容器，剩下的为物品栏
	static constexpr size_t szContainerIndexBeg = 5;
	static inline const NBT_Node::NBT_String sSlotTagName[] =
	{
		MU8STR("ArmorItems"),//0
		MU8STR("HandItems"),
		MU8STR("Inventory"),//2
		MU8STR("SaddleItem"),//3
		MU8STR("DecorItem"),

		MU8STR("Items"),//5
		MU8STR("Item"),
		//MU8STR(""),
	};//此数组不能乱改，有索引强相关！

	//掉落物和物品展示框这些都算Item，会走实体容器处理，符合要求，无需特判

	struct EntityItemSlot
	{
		size_t szSlotTagNameIndex{};
		const NBT_Node *pItems{};
	};

	struct EntityStats
	{
		const NBT_Node::NBT_String *psEntityName{};
		std::vector<EntityItemSlot> listSlot;
	};

	struct EntitySlot
	{
		ItemProcess::ItemStackList listContainer;
		ItemProcess::ItemStackList listInventory;
	};

	using EntityStatsList = std::vector<EntityStats>;

private:
	/*	
	马的鞍是通过SaddleItem的compound里面的物品决定的，而猪、赤足兽这些这则是通过Saddle的bool标签决定的
	得进行特判，然后将bool转换到鞍物品进行统计
	
	如果实体有带拴绳，则存在Leash这个compound标签，内部有拴绳坐标或者是拉着他的实体的uuid，总之不为空，这种情况下转换为拴绳
	*/
	//查找并转换那些特殊的数据值到物品
	static void ExtractSpecial(std::vector<EntityItemSlot> &listSlot, const NBT_Node::NBT_Compound& cpdEntity)
	{
		//先对两个进行查找，然后判断
		const auto pSaddle = cpdEntity.HasByte(MU8STR("Saddle"));
		const auto pLeash = cpdEntity.HasCompound(MU8STR("Leash"));

		//声明两个静态的成员，让EntityStats的指针指向它，伪装成正常读取的数据（不会被改写）
		using CP = std::pair<const NBT_Node::NBT_String, NBT_Node>;
		static const NBT_Node slotSaddleItem
		{
			NBT_Node::NBT_Compound
			{
				CP{NBT_Node::NBT_String{MU8STR("id")},NBT_Node::NBT_String{MU8STR("minecraft:saddle")}},
				CP{NBT_Node::NBT_String{MU8STR("Count")},NBT_Node::NBT_Byte{1}},
			}
		};
		static const NBT_Node slotLeadItem
		{
			NBT_Node::NBT_Compound
			{
				CP{NBT_Node::NBT_String{MU8STR("id")},NBT_Node::NBT_String{MU8STR("minecraft:lead")}},
				CP{NBT_Node::NBT_String{MU8STR("Count")},NBT_Node::NBT_Byte{1}},
			}
		};

		//有鞍，加一个
		if (pSaddle != NULL && *pSaddle != 0)//以byte存储的bool值不为0
		{
			listSlot.emplace_back(3, &slotSaddleItem);//这里的3就是上面数组的鞍位置
		}

		//有拴绳信息并且信息非空（只要非空即可，是什么（比如坐标或者牵拉对象uuid）都不重要），加一个
		if (pLeash != NULL && pLeash->size() != 0)//因为拴绳只在目标对象存储，拉拴绳的主体不存储，不会重复计算
		{
			listSlot.emplace_back(2, &slotLeadItem);//因为拴绳不属于任何一个实体物品栏类型，所以简单归属到物品栏（2）
		}

	}

public:
	static EntityStatsList GetEntityStats(const NBT_Node::NBT_Compound &RgCompound)
	{
		//获取实体列表
		const auto &listEntity = RgCompound.GetList(MU8STR("Entities"));
		
		//遍历，并在每个实体compound下查询所有关键字进行分类
		EntityStatsList listEntityStatsList{};
		listEntityStatsList.reserve(listEntity.size());//提前扩容
		//遍历实体
		for (const auto &it : listEntity)
		{
			//转换类型
			const auto &curEntity = GetCompound(it);
			
			//在每个entity内查找所有可能出现的可以容纳物品的tag
			EntityStats stEntityStats{ &curEntity.GetString(MU8STR("id")) };//先获取实体名字
			//转换特殊的实体id数据值到物品
			ExtractSpecial(stEntityStats.listSlot, curEntity);

			//遍历所有可以存放物品的格子名字
			for (const auto &itTag : sSlotTagName)
			{
				const auto pSearch = curEntity.Search(itTag);//并在实体compound内查询，如果找到代表存在
				if (pSearch == NULL)
				{
					continue;//没有这个tag，跳过
				}

				//把找到的物品栏集合放入集合列表
				stEntityStats.listSlot.emplace_back((&itTag - sSlotTagName), pSearch);//偏移地址减去基地址获取下标
			}
			
			//最后把带有一个实体所有物品栏的信息放列表
			listEntityStatsList.push_back(std::move(stEntityStats));
		}

		//返回给调用者，以供下一步处理
		return listEntityStatsList;
	}

	//这个真的是我写到现在最简单的一个函数了（蚌埠住）
	static EntityInfo EntityStatsToEntity(const EntityStats &stEntityStats)
	{
		return EntityInfo{ *stEntityStats.psEntityName };
	}

	/*
	下面的目前不处理，因为一般情况下除非故意指令刷否则都是正常掉落的重力方块
	草了，掉落的方块这个实体，如果内部有标签，并且是方块实体，nbt标签居然是叫blockentity而不是tileentity，
	而且还有blockstate，估计得走blockprocess代理处理转换
	注意如果是方块展示实体，会存在block_state
	*/
	//static ItemStackList EntityStatsToItemStack(const EntityStats &stEntityStats)//代办
	//{
	//
	//}


/*
	需要把那些物品展示实体、方块展示实体等的内部nbt tag排除掉，因为并不是可以塞物品进去形成，而是只能命令刷出的

	部分实体需要转换为物品形式，不能转换的直接显示实体信息->此处代办，所有物品暂时先全部输出实体信息
	比如漏斗矿车、盔甲架、掉落物、物品展示框等
*/

	static EntitySlot EntityStatsToEntitySlot(const EntityStats &stEntityStats)
	{
		if (*stEntityStats.psEntityName == MU8STR("minecraft:item_display"))//跳过
		{
			return {};//空
		}

		EntitySlot stEntitySlot{};
		for (const auto &it : stEntityStats.listSlot)
		{
			ItemProcess::ItemStackList *pCurList{};

			//说明是物品栏
			if (it.szSlotTagNameIndex < szContainerIndexBeg)
			{
				pCurList = &stEntitySlot.listInventory;
			}
			else//说明是容器
			{
				pCurList = &stEntitySlot.listContainer;
			}
			

			//获取tag后根据实际类型进行解析
			const auto tag = it.pItems->GetTag();
			if (tag == NBT_Node::TAG_Compound)
			{
				if (it.pItems->GetCompound().empty())
				{
					continue;//空tag，跳过
				}
				pCurList->push_back(ItemProcess::ItemCompoundToItemStack(it.pItems->GetCompound()));
			}
			else if (tag == NBT_Node::TAG_List)
			{
				const auto &tmp = it.pItems->GetList();
				for (const auto &cur : tmp)
				{
					if (cur.GetCompound().empty())
					{
						continue;//空tag，跳过
					}
					pCurList->push_back(ItemProcess::ItemCompoundToItemStack(cur.GetCompound()));
				}
			}
			//else {}
		}

		return stEntitySlot;
	}
};