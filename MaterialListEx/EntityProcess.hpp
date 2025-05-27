#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"
#include "ItemProcess.hpp"

#include <compare>

struct EntityInfo
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

	//5~6Ϊ������ʣ�µ�Ϊ��Ʒ��
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
	};//�����鲻���Ҹģ�������ǿ��أ�

	//���������Ʒչʾ����Щ����Item������ʵ��������������Ҫ����������

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
	��İ���ͨ��SaddleItem��compound�������Ʒ�����ģ�������������Щ������ͨ��Saddle��bool��ǩ������
	�ý������У�Ȼ��boolת��������Ʒ����ͳ��
	
	���ʵ���д�˩���������Leash���compound��ǩ���ڲ���˩�������������������ʵ���uuid����֮��Ϊ�գ����������ת��Ϊ˩��
	*/
	//���Ҳ�ת����Щ���������ֵ����Ʒ
	static void ExtractSpecial(std::vector<EntityItemSlot> &listSlot, const NBT_Node::NBT_Compound& cpdEntity)
	{
		//�ȶ��������в��ң�Ȼ���ж�
		const auto pSaddle = cpdEntity.HasByte(MU8STR("Saddle"));
		const auto pLeash = cpdEntity.HasCompound(MU8STR("Leash"));

		//����������̬�ĳ�Ա����EntityStats��ָ��ָ������αװ��������ȡ�����ݣ����ᱻ��д��
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

		//�а�����һ��
		if (pSaddle != NULL && *pSaddle != 0)//��byte�洢��boolֵ��Ϊ0
		{
			listSlot.emplace_back(3, &slotSaddleItem);//�����3������������İ�λ��
		}

		//��˩����Ϣ������Ϣ�ǿգ�ֻҪ�ǿռ��ɣ���ʲô�������������ǣ������uuid��������Ҫ������һ��
		if (pLeash != NULL && pLeash->size() != 0)//��Ϊ˩��ֻ��Ŀ�����洢����˩�������岻�洢�������ظ�����
		{
			listSlot.emplace_back(2, &slotLeadItem);//��Ϊ˩���������κ�һ��ʵ����Ʒ�����ͣ����Լ򵥹�������Ʒ����2��
		}

	}

public:
	static EntityStatsList GetEntityStats(const NBT_Node::NBT_Compound &RgCompound)
	{
		//��ȡʵ���б�
		const auto &listEntity = RgCompound.GetList(MU8STR("Entities"));
		
		//����������ÿ��ʵ��compound�²�ѯ���йؼ��ֽ��з���
		EntityStatsList listEntityStatsList{};
		listEntityStatsList.reserve(listEntity.size());//��ǰ����
		//����ʵ��
		for (const auto &it : listEntity)
		{
			//ת������
			const auto &curEntity = GetCompound(it);
			
			//��ÿ��entity�ڲ������п��ܳ��ֵĿ���������Ʒ��tag
			EntityStats stEntityStats{ &curEntity.GetString(MU8STR("id")) };//�Ȼ�ȡʵ������
			//ת�������ʵ��id����ֵ����Ʒ
			ExtractSpecial(stEntityStats.listSlot, curEntity);

			//�������п��Դ����Ʒ�ĸ�������
			for (const auto &itTag : sSlotTagName)
			{
				const auto pSearch = curEntity.Search(itTag);//����ʵ��compound�ڲ�ѯ������ҵ��������
				if (pSearch == NULL)
				{
					continue;//û�����tag������
				}

				//���ҵ�����Ʒ�����Ϸ��뼯���б�
				stEntityStats.listSlot.emplace_back((&itTag - sSlotTagName), pSearch);//ƫ�Ƶ�ַ��ȥ����ַ��ȡ�±�
			}
			
			//���Ѵ���һ��ʵ��������Ʒ������Ϣ���б�
			listEntityStatsList.push_back(std::move(stEntityStats));
		}

		//���ظ������ߣ��Թ���һ������
		return listEntityStatsList;
	}

	//����������д��������򵥵�һ�������ˣ�����ס��
	static EntityInfo EntityStatsToEntity(const EntityStats &stEntityStats)
	{
		return EntityInfo{ *stEntityStats.psEntityName };
	}

	/*
	�����Ŀǰ��������Ϊһ������³��ǹ���ָ��ˢ�����������������������
	���ˣ�����ķ������ʵ�壬����ڲ��б�ǩ�������Ƿ���ʵ�壬nbt��ǩ��Ȼ�ǽ�blockentity������tileentity��
	���һ���blockstate�����Ƶ���blockprocess������ת��
	ע������Ƿ���չʾʵ�壬�����block_state
	*/
	//static ItemStackList EntityStatsToItemStack(const EntityStats &stEntityStats)//����
	//{
	//
	//}


/*
	��Ҫ����Щ��Ʒչʾʵ�塢����չʾʵ��ȵ��ڲ�nbt tag�ų�������Ϊ�����ǿ�������Ʒ��ȥ�γɣ�����ֻ������ˢ����

	����ʵ����Ҫת��Ϊ��Ʒ��ʽ������ת����ֱ����ʾʵ����Ϣ->�˴����죬������Ʒ��ʱ��ȫ�����ʵ����Ϣ
	����©���󳵡����׼ܡ��������Ʒչʾ���
*/

	static EntitySlot EntityStatsToEntitySlot(const EntityStats &stEntityStats)
	{
		if (*stEntityStats.psEntityName == MU8STR("minecraft:item_display"))//����
		{
			return {};//��
		}

		EntitySlot stEntitySlot{};
		for (const auto &it : stEntityStats.listSlot)
		{
			ItemProcess::ItemStackList *pCurList{};

			//˵������Ʒ��
			if (it.szSlotTagNameIndex < szContainerIndexBeg)
			{
				pCurList = &stEntitySlot.listInventory;
			}
			else//˵��������
			{
				pCurList = &stEntitySlot.listContainer;
			}
			

			//��ȡtag�����ʵ�����ͽ��н���
			const auto tag = it.pItems->GetTag();
			if (tag == NBT_Node::TAG_Compound)
			{
				if (it.pItems->GetCompound().empty())
				{
					continue;//��tag������
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
						continue;//��tag������
					}
					pCurList->push_back(ItemProcess::ItemCompoundToItemStack(cur.GetCompound()));
				}
			}
			//else {}
		}

		return stEntitySlot;
	}
};