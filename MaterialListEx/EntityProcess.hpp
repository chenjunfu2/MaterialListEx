#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"

#include <compare>

struct Entity
{
	NBT_Node::NBT_String sName{};
	//���������⣬ʣ��ȫ���ˣ�û����
	uint64_t u64Hash{ DataHash() };

public:
	uint64_t DataHash(void)
	{
		static_assert(std::is_same_v<XXH64_hash_t, uint64_t>, "Hash type does not match the required type.");

		constexpr static XXH64_hash_t HASH_SEED = 0x83B0'1A83'062C'4F5D;

		return XXH64(sName.data(), sName.size(), HASH_SEED);
	}

public:
	static size_t Hash(const Entity &self)
	{
		return self.u64Hash;
	}

	static bool Equal(const Entity &_l, const Entity &_r)
	{
		return _l.u64Hash == _r.u64Hash && _l.sName == _r.sName;
	}

	inline std::strong_ordering operator<=>(const Entity &_r) const
	{
		//�Ȱ��չ�ϣ��
		if (auto tmp = (u64Hash <=> _r.u64Hash); tmp != 0)
		{
			return tmp;
		}

		//����������
		return sName <=> _r.sName;
	}
};

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

	struct ItemStack
	{
		NBT_Node::NBT_String sItemName{};//��Ʒ��
		NBT_Node::NBT_Compound cpdItemTag{};//��Ʒ��ǩ
		NBT_Node::NBT_Byte byteItemCount = 0;//��Ʒ������
	};

	using EntityStatsList = std::vector<EntityStats>;
	using ItemStackList = std::vector<ItemStack>;
public:
	static EntityStatsList GetEntityStats(const NBT_Node::NBT_Compound &RgCompound)
	{
		/*
			��Ҫ����ʵ����Ʒ����ʵ������
			ת�����棬��Ҫ����Щ��Ʒչʾʵ�塢����չʾʵ��ȵ��ڲ�nbt tag�ų�������Ϊ�����ǿ�������Ʒ��ȥ�γɣ�����ֻ������ˢ����
			�����ų�����ɾ����ת������Ϊʵ����Ʒ�����֣�����ʵ����������
					
			����ʵ����Ҫת��Ϊ��Ʒ��ʽ������ת����ֱ����ʾʵ����Ϣ
			����©���󳵡����׼ܡ��������Ʒչʾ���
			
			ʵ����������Ϣ���
			
			ʵ����Ʒ�����
			
			��Ʒ��Ҫ����ҩ����ҩˮ����ħ����֮���ˡ��̻�����ȼ���ʽ��������ʽ����֪�ɶ�NBT��Ϣ���и�ʽ�����
			���ڿ���Ҫ��Ҫ�Ѵ�˩����ʵ���˩������ͳ��
			
			���ˣ���İ���ͨ��SaddleItem��compound�������Ʒ�����ģ�������������Щ������ͨ��Saddle��bool��ǩ������
			�ý������У�Ȼ��boolת��������Ʒ����ͳ��
			
			���ˣ�����ķ������ʵ�壬����ڲ��б�ǩ�������Ƿ���ʵ�壬nbt��ǩ��Ȼ�ǽ�blockentity������tileentity��
			���һ���blockstate�����Ƶ���blockprocess������ת��
		*/

		//��ȡʵ���б�
		const auto &listEntity = RgCompound.GetList(MU8STR("Entities"));
		
		//����������ÿ��ʵ��compound�²�ѯ���йؼ��ֽ��з���
		EntityStatsList listEntityStatsList{};
		listEntityStatsList.reserve(listEntity.size());//��ǰ����
		for (const auto &it : listEntity)
		{
			const auto &cur = GetCompound(it);
			
			//��ÿ��entity�ڲ������п��ܳ��ֵĿ���������Ʒ��tag
			EntityStats stEntityStats{ &cur.GetString(MU8STR("id")) };//�Ȼ�ȡʵ������
			for (const auto &itTag : sSlotTagName)//�������п��Դ����Ʒ�ĸ�������
			{
				const auto pSearch = cur.Search(itTag);//����ʵ��compound�ڲ�ѯ������ҵ��������
				if (pSearch == NULL)
				{
					continue;//û�����tag������
				}

				//���ҵ�����Ʒ�����Ϸ��뼯���б�
				stEntityStats.listSlot.emplace_back(&itTag, pSearch);
			}
			
			//���Ѵ���һ��ʵ��������Ʒ������Ϣ���б�
			listEntityStatsList.push_back(std::move(stEntityStats));
		}

		//���ظ������ߣ��Թ���һ������
		return listEntityStatsList;
	}

	//����������д��������򵥵�һ�������ˣ�����ס��
	static Entity EntityStatsToEntity(const EntityStats &stEntityStats)
	{
		return Entity{ *stEntityStats.psEntityName };
	}

	static ItemStackList EntityStatsContainerToItemStack(const EntityStats &stEntityStats)
	{





	}

	static ItemStackList EntityStatsInventoryToItemStack(const EntityStats &stEntityStats)
	{





	}


};