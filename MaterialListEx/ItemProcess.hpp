#pragma once

#include "NBT_Node.hpp"
#include "NBT_Helper.hpp"

#include <xxhash.h>
#include <compare>

struct ItemInfo
{
	NBT_Node::NBT_String sName{};
	NBT_Node::NBT_Compound cpdTag{};
	uint64_t u64Hash{ DataHash() };//��ʼ��˳���ϸ�������˳�򣬴˴�������

public:
	uint64_t DataHash(void)
	{
		static_assert(std::is_same_v<XXH64_hash_t, uint64_t>, "Hash type does not match the required type.");

		constexpr static XXH64_hash_t HASH_SEED = 0xDE35'B92A'7F41'806C;

		if (cpdTag.empty())//tagΪ��ֻ��������
		{
			return XXH64(sName.data(), sName.size(), HASH_SEED);
		}
		else//��Ϊ�������tag
		{
			return NBT_Helper::Hash(cpdTag, HASH_SEED,
				[this](XXH64_state_t *pHashState) -> void//��tag֮ǰ��������
				{
					XXH64_update(pHashState, this->sName.data(), this->sName.size());
				});
		}
	}

public:
	static size_t Hash(const ItemInfo &self)
	{
		return self.u64Hash;
	}

	static bool Equal(const ItemInfo &_l, const ItemInfo &_r)
	{
		return _l.u64Hash == _r.u64Hash && _l.sName == _r.sName && _l.cpdTag == _r.cpdTag;
	}

	inline std::partial_ordering operator<=>(const ItemInfo &_r) const
	{
		//�Ȱ��չ�ϣ��
		if (auto tmp = (u64Hash <=> _r.u64Hash); tmp != 0)
		{
			return tmp;
		}

		//����������
		if (auto tmp = (sName <=> _r.sName); tmp != 0)
		{
			return tmp;
		}

		//����ͬ�����Tag��
		return cpdTag <=> _r.cpdTag;
	}
};

/*
��Ʒ��Ҫ����ҩ����ҩˮ����ħ����֮���ˡ��̻�����ȼ���ʽ��������ʽ����֪�ɶ�NBT��Ϣ���и�ʽ�����
�����ѷ���ʵ�������Ʒ�Ĵ����߼��ƶ����ˣ�����ʵ������Ʒ����Ҳ��������
ͻȻ������һ���ش����⣬�������������Ϣ����Ʒ�ѵ��ˣ�Count��Ϊ1�������������е����ж������������ó������Ķѵ����ʣ�

ֻ�Բ��ִ�������ʵ����Ϣ��ʵ��ݹ�����ǱӰ�У����ɴ�

*/
class ItemProcess
{
public:
	ItemProcess() = delete;
	~ItemProcess() = delete;

	struct ItemStack
	{
		NBT_Node::NBT_String sItemName{};//��Ʒ��
		NBT_Node::NBT_Compound cpdItemTag{};//��Ʒ��ǩ
		NBT_Node::NBT_Byte byteItemCount = 0;//��Ʒ������
	};

	using ItemStackList = std::vector<ItemStack>;

	struct NoTagItem
	{
		NBT_Node::NBT_String sItemName{};//��Ʒ��
		uint64_t u64Counter = 0;//��Ʒ������
	};

	using NoTagItemList = std::vector<NoTagItem>;

public:
	/*
	��һ���ش����⣬������ʵ��ѵ��ˣ�count��Ϊ1���������ʵ���������������������е����ж������ó������Ķѵ����ʣ�
	*/
	//Ҫ���û�����ԭʼlistItemStack������Ȩ
	static ItemStackList ItemStackListUnpackContainer(ItemStackList &&listItemStack, size_t szStackDepth = 64)
	{
		ItemStackList ret;
		ret.reserve(listItemStack.size());//��ǰ���ݣ����ǽ��������¿��ܻ�������µĶ�����
		for (auto &it : listItemStack)
		{
			ItemStackUnpackContainer(std::move(it), ret, szStackDepth);
		}
		return ret;
	}

	static ItemStack ItemCompoundToItemStack(const NBT_Node::NBT_Compound &cpdItem)
	{
		auto cpdTag = cpdItem.HasCompound(MU8STR("tag"));
		return ItemStack
		{
			cpdItem.GetString(MU8STR("id")),
			cpdTag == NULL ? NBT_Node::NBT_Compound{} : *cpdTag,
			cpdItem.GetByte(MU8STR("Count"))
		};
	}

	static ItemStack ItemCompoundToItemStack(NBT_Node::NBT_Compound &&cpdItem)
	{
		auto cpdTag = cpdItem.HasCompound(MU8STR("tag"));
		return ItemStack
		{
			std::move(cpdItem.GetString(MU8STR("id"))),
			cpdTag == NULL ? NBT_Node::NBT_Compound{} : std::move(*cpdTag),
			std::move(cpdItem.GetByte(MU8STR("Count")))
		};
	}

	//TODO:�ӷ���ʵ���Ǳ߰��߼�������Ȼ���޸�������ʹ��item������ÿ��������ת�������һ�Σ������������������tag��
private:
	/*
		ֻ�����֪���飨����ǱӰ��+���ɴ�������֮��ȫ���ų�
		Ϊʲô�������nbt�ķ���ʵ�壿��Ϊ�ⶫ��ֻ��ͨ��ָ��ʹ����ã�û�취��
		��������һ���Ž�ȥ�����Һܶ෽��ʵ��洢��ʱ�򲻴�����ʵ��id���ø�������ת��
		�ǳ��鷳�������������ô���ˣ���ô��nbt������ʵ�壨����©�������֣�
		Ϊ���뷽��ʵ��һ��Ҳ��Ҫ�⣬����ˢ�ֵ�������ʵ��������Ҳ���Ժ���Ʒ�����ǲ���ҲҪ�⣿
		Խ��Խ�����ˣ������ֶ�������ֻ��ͨ��ָ���ã���ȫû��Ҫ����ֱ�ӱ���ԭ��ʽ��nbt�������
	*/

	static void ItemStackUnpackContainer(ItemStack &&stItemStack, ItemStackList &listItemStack, size_t szStackDepth)
	{
		if (szStackDepth <= 0)
		{
			return;//ջ��ȳ��ˣ�ֱ�ӷ���
		}

		if (stItemStack.cpdItemTag.empty())//����û��tag�����治���ж���
		{
			goto push_return;
		}

		//��tag������¿����ǲ�����Ҫ������������Ǿ͵ݹ�⣬���򷵻�
		//��ΪǱӰ����n������ɫ������ǰ׺��ʽ�������жϺ�׺ȷ���ǲ���ǱӰ��
		//Ϊʲô���жϷ���ʵ��id����������ô����֣�����������˭���齫����ĺܣ��еķ���ʵ������������id
		if (stItemStack.sItemName.ends_with(MU8STR("shulker_box")))
		{
			//����tag����û��BlockEntityTag��û������뷵��
			auto pcpdBlockEntityTag = stItemStack.cpdItemTag.HasCompound(MU8STR("BlockEntityTag"));
			if (pcpdBlockEntityTag == NULL)
			{
				goto push_return;
			}

			//�������ڲ���Items��ǱӰ���ڲ�ֻ����������������ˣ�
			auto pcpdItems = pcpdBlockEntityTag->Search(MU8STR("Items"));//���ﲻ��HasXXX����Search��Ϊ�˲���������ͣ�����TraversalItems����
			TraversalItems(pcpdItems, listItemStack, szStackDepth);
		}
		else if (stItemStack.sItemName == MU8STR("minecraft:bundle"))//���ɴ����⴦��
		{
			//ֱ���ҵ�Items
			auto plistItems = stItemStack.cpdItemTag.Search(MU8STR("Items"));//���ﲻ��HasXXX����Search��Ϊ�˲���������ͣ�����TraversalItems����
			TraversalItems(plistItems, listItemStack, szStackDepth);
		}
		//else {} //���漸�������ǣ���������ֱ�Ӳ������

		
	push_return:
		//������ζ�Ҫ���в��룬���ǰ������˽����ô�������Ʒ��nbt���̣�����ֱ�Ӳ���
		listItemStack.push_back(std::move(stItemStack));
		return;
	}

	static void TraversalItems(NBT_Node *pItems, ItemStackList &listItemStack, size_t szStackDepth)
	{
		if (pItems == NULL)
		{
			return;//Ϊ��ֱ�ӷ���
		}

		//�ж�����
		auto tag = pItems->GetTag();
		if (tag == NBT_Node::TAG_Compound)
		{
			auto &tmp = pItems->GetCompound();
			ItemStack tmpItem = ItemCompoundToItemStack(std::move(tmp));
			ItemStackUnpackContainer(std::move(tmpItem), listItemStack, szStackDepth - 1);//�ݹ� - 1
			pItems->GetCompound().clear();//��գ��ڲ������Ѿ�ת������Ȩ����Ӧ����ʹ�ã�
		}
		else if (tag == NBT_Node::TAG_List)
		{
			auto &tmp = pItems->GetList();
			for (auto &it : tmp)//����list
			{
				ItemStack tmpItem = ItemCompoundToItemStack(std::move(it.GetCompound()));
				ItemStackUnpackContainer(std::move(tmpItem), listItemStack, szStackDepth - 1);//�ݹ� - 1
			}
			tmp.clear();//��գ��ڲ������Ѿ�ת������Ȩ����Ӧ����ʹ�ã�
		}
		//else {}//����

		return;
	}
};