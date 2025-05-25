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

	struct BlockItemStack
	{
		NBT_Node::NBT_String sItemName{};//��Ʒ��
		uint64_t u64Counter = 0;//��Ʒ������
	};

	using BlockItemStackList = std::vector<BlockItemStack>;

public:
	/*
	��һ���ش����⣬������ʵ��ѵ��ˣ�count��Ϊ1���������ʵ���������������������е����ж������ó������Ķѵ����ʣ�
	*/
	ItemStackList ItemStackUnpackToItemList(const ItemStack &stItem)
	{





	}

	//TODO:�ӷ���ʵ���Ǳ߰��߼�������Ȼ���޸�������ʹ��item������ÿ��������ת�������һ�Σ������������������tag��
};