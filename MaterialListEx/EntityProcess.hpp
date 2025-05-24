#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"

class EntityProcess
{
public:
	EntityProcess() = delete;
	~EntityProcess() = delete;

	static inline const NBT_Node::NBT_String sSlotTagName[] =
	{
		MU8STR("ArmorItems"),
		MU8STR("HandItems"),
		MU8STR("Inventory"),
		MU8STR("SaddleItem"),
		MU8STR("DecorItem"),
		MU8STR("Items"),
		MU8STR("Item"),
		//MU8STR(""),
	};


	struct Items
	{
		const NBT_Node::NBT_String *pSlotTag{};
		const NBT_Node *pItems{};
	};

	struct EntityStats
	{
		const NBT_Node::NBT_String *psEntityName{};
		std::vector<Items> listSlot;
	};


	using EntityStatsList = std::vector<EntityStats>;

public:
	static EntityStatsList GetEntityStats(const NBT_Node::NBT_Compound &RgCompound)
	{
		/*
			需要处理实体物品栏和实体容器
			转换方面，需要把那些物品展示实体、方块展示实体等的内部nbt tag排除掉，因为并不是可以塞物品进去形成，而是只能命令刷出的
			但是排除而不删除，转换并作为实体物品栏出现，而非实体容器出现
					
			部分实体需要转换为物品形式，不能转换的直接显示实体信息
			比如漏斗矿车、盔甲架、掉落物、物品展示框等
			
			实体内容器信息解包
			
			实体物品栏解包
			
			物品需要解析药箭、药水、附魔、谜之炖菜、烟花火箭等级样式，旗帜样式等已知可读NBT信息进行格式化输出
			还在考虑要不要把带拴绳的实体的拴绳进行统计
			
			绝了，马的鞍是通过SaddleItem的compound里面的物品决定的，而猪、赤足兽这些这则是通过Saddle的bool标签决定的
			得进行特判，然后将bool转换到鞍物品进行统计
			
			草了，掉落的方块这个实体，如果内部有标签，并且是方块实体，nbt标签居然是叫blockentity而不是tileentity，
			而且还有blockstate，估计得走blockprocess代理处理转换
		*/

		//获取实体列表
		const auto &listEntity = RgCompound.GetList(MU8STR("Entities"));
		
		//遍历，并在每个实体compound下查询所有关键字进行分类
		EntityStatsList listEntityStatsList{};
		listEntityStatsList.reserve(listEntity.size());//提前扩容
		for (const auto &it : listEntity)
		{
			const auto &cur = GetCompound(it);
			
			//在每个entity内查找所有可能出现的可以容纳物品的tag
			EntityStats stEntityStats{ &cur.GetString(MU8STR("id")) };//先获取实体名字
			for (const auto &itTag : sSlotTagName)//遍历所有可以存放物品的格子名字
			{
				const auto pSearch = cur.Search(itTag);//并在实体compound内查询，如果找到代表存在
				if (pSearch == NULL)
				{
					continue;//没有这个tag，跳过
				}

				//把找到的物品栏集合放入集合列表
				stEntityStats.listSlot.emplace_back(&itTag, pSearch);
			}
			
			//最后把带有一个实体所有物品栏的信息放列表
			listEntityStatsList.push_back(std::move(stEntityStats));
		}

		//返回给调用者，以供下一步处理
		return listEntityStatsList;
	}


	





};