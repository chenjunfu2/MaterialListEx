#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"

class EntityProcess
{
public:
	EntityProcess() = delete;
	~EntityProcess() = delete;


	struct EntityStats
	{
		const NBT_Node::NBT_String *psEntityName{};

		static constexpr const char *const sSlotContainerTag[] =
		{
			MU8STR("ArmorItems"),
			MU8STR("HandItems"),
			MU8STR("Inventory"),
			MU8STR("SaddleItem"),
			MU8STR("DecorItem"),
			MU8STR("Items"),
			MU8STR("Item"),
			//MU8STR(""),
			//MU8STR(""),
			//MU8STR(""),
			//MU8STR(""),
		};

		struct Items
		{
			const NBT_Node *pItems{};
			NBT_Node::NBT_TAG enType{ NBT_Node::TAG_End };
		};

		std::vector<Items> listInventory;
		std::vector<Items> listContainer;
	};






	using EntityStatsList = std::vector<EntityStats>;

public:
	static EntityStatsList GetEntityStats(const NBT_Node::NBT_Compound &RgCompound)
	{
		const auto &listEntity = RgCompound.GetList(MU8STR("Entities"));

		//需要处理实体物品栏和实体容器
		//转换方面，需要把那些物品展示实体、方块展示实体等的内部nbt tag排除掉，因为并不是可以塞进去的，而是只能命令刷出的
		//但是排除而不删除，转换并作为实体物品栏出现，而非实体容器出现




		return {};
	}


	





};