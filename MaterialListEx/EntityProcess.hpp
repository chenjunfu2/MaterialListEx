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

		//��Ҫ����ʵ����Ʒ����ʵ������
		//ת�����棬��Ҫ����Щ��Ʒչʾʵ�塢����չʾʵ��ȵ��ڲ�nbt tag�ų�������Ϊ�����ǿ�������ȥ�ģ�����ֻ������ˢ����
		//�����ų�����ɾ����ת������Ϊʵ����Ʒ�����֣�����ʵ����������




		return {};
	}


	





};