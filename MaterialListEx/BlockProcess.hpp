#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"
#include "Calc_Tool.hpp"
#include "ItemProcess.hpp"

#include <stdint.h>
#include <vector>
#include <unordered_set>
#include <unordered_map>


//����Ʒ���ƣ�ͬ���������
using BlockInfo = ItemInfo;

class BlockProcess
{
public:
	BlockProcess() = delete;
	~BlockProcess() = delete;

	//���˿ɻ�ȡ�ķ��飬���ɻ�ȡ�ķ���ֱ��ɾ������ש���������Ʒ����ת������
	//(Ӧ�Ի��衢��ˮ���顢��ҩ�������򵰸�������Ʒ)
	struct BlockStats
	{
		const NBT_Node::NBT_String *psBlockName{};
		const NBT_Node::NBT_Compound *pcpdProperties{};
		uint64_t u64Counter = 0;//���������
	};

	using BlockStatsList = std::vector<BlockStats>;

public:
	//ʹ��ָ��ָ��nRoot�ڽڵ㣬�벻Ҫ��ʹ��nRoot��Ϊ�������ô˺�����ȡ����ֵ������£�����nRoot
	static BlockStatsList GetBlockStats(const NBT_Node::NBT_Compound &RgCompound)//block list
	{
		/*----------------�����С���㡢��ɫ���ȡ----------------*/
		//��ȡ����ƫ��
		const auto &Position = RgCompound.GetCompound(MU8STR("Position"));
		const BlockPos reginoPos =
		{
			.x = Position.GetInt(MU8STR("x")),
			.y = Position.GetInt(MU8STR("y")),
			.z = Position.GetInt(MU8STR("z")),
		};
		//��ȡ�����С������Ϊ�����������ƫ�Ƽ���ʵ�ʴ�С��
		const auto &Size = RgCompound.GetCompound(MU8STR("Size"));
		const BlockPos regionSize =
		{
			.x = Size.GetInt(MU8STR("x")),
			.y = Size.GetInt(MU8STR("y")),
			.z = Size.GetInt(MU8STR("z")),
		};
		//��������ʵ�ʴ�С
		const BlockPos posEndRel = getRelativeEndPositionFromAreaSize(regionSize).add(reginoPos);
		const BlockPos posMin = getMinCorner(reginoPos, posEndRel);
		const BlockPos posMax = getMaxCorner(reginoPos, posEndRel);
		const BlockPos size = posMax.sub(posMin).add({ 1,1,1 });

		//printf("RegionSize: [%d, %d, %d]\n", size.x, size.y, size.z);

		//��ȡ��ɫ�壨�������ࣩ
		const auto &BlockStatePalette = RgCompound.GetList(MU8STR("BlockStatePalette"));
		const uint32_t bitsPerBitMapElement = Max(2U, (uint32_t)sizeof(uint32_t) * 8 - numberOfLeadingZeros(BlockStatePalette.size() - 1));//����λͼ��һ��Ԫ��ռ�õ�bit��С
		const uint32_t bitMaskOfElement = (1 << bitsPerBitMapElement) - 1;//��ȡ����λ������ȡbitmap�ڲ�����
		//printf("BlockStatePaletteSize: [%zu]\nbitsPerBitMapElement: [%d]\n", BlockStatePalette.size(), bitsPerBitMapElement);
		/*------------------------------------------------*/

		/*----------------���ݷ���λͼ���ʵ�ɫ�壬��ȡ��ͬ״̬����ļ���----------------*/
		//��������ͳ��vector
		BlockStatsList listBlockStats;
		listBlockStats.reserve(BlockStatePalette.size());//��ǰ����

		//����BlockStatePalette�����ɫ�壬�����д�����Ч�±�ķ���ͳ��vector
		for (const auto &it : BlockStatePalette)
		{
			const auto &itBlockCompound = GetCompound(it);

			BlockStats bsTemp{};
			bsTemp.psBlockName = &itBlockCompound.GetString(MU8STR("Name"));
			const auto find = itBlockCompound.HasCompound(MU8STR("Properties"));//��鷽���Ƿ��ж�������
			if (find != NULL)
			{
				bsTemp.pcpdProperties = find;
			}
			else
			{
				bsTemp.pcpdProperties = NULL;
			}

			listBlockStats.emplace_back(std::move(bsTemp));
		}

		//��ȡLong����״̬λͼ���飨������Ϊ�±���ʵ�ɫ�壩
		const auto &BlockStates = RgCompound.GetLongArray(MU8STR("BlockStates"));
		const uint64_t RegionFullSize = (uint64_t)size.x * (uint64_t)size.y * (uint64_t)size.z;
		if (BlockStates.size() * 64 / bitsPerBitMapElement < RegionFullSize)
		{
			//printf("BlockStates Too Small!\n");
			return listBlockStats;
		}

		//�洢��ʽ��(y * sizeLayer) + z * sizeX + x = Long array index, sizeLayer = sizeX * sizeZ
		//��������λͼ�����õ�ɫ���б��Ӧ����ļ�����
		for (uint64_t startOffset = 0; startOffset < (RegionFullSize * (uint64_t)bitsPerBitMapElement); startOffset += (uint64_t)bitsPerBitMapElement)//startOffset�ӵڶ��ο�ʼ����
		{
			const uint64_t startArrIndex = (uint64_t)(startOffset >> 6);//div 64
			const uint64_t startBitOffset = (uint64_t)(startOffset & 0x3F);//mod 64
			const uint64_t endArrIndex = (uint64_t)((startOffset + (uint64_t)bitsPerBitMapElement - 1ULL) >> 6);//div 64
			uint64_t paletteIndex = 0;

			if (startArrIndex == endArrIndex)
			{
				paletteIndex = ((uint64_t)BlockStates[startArrIndex] >> startBitOffset) & bitMaskOfElement;
			}
			else
			{
				const uint64_t endBitOffset = sizeof(uint64_t) * 8 - startBitOffset;
				paletteIndex = ((uint64_t)BlockStates[startArrIndex] >> startBitOffset | (uint64_t)BlockStates[endArrIndex] << endBitOffset) & bitMaskOfElement;
			}

			++listBlockStats[paletteIndex].u64Counter;
		}
		/*------------------------------------------------*/

		//������ɷ���
		return listBlockStats;
	}

	static ItemProcess::NoTagItemList BlockStatsToItemStack(const BlockStats &stBlocks)
	{
		//�����鵽��Ʒת��
		ItemProcess::NoTagItemList stItemsList{};

		//��鷽����
		if (stBlocks.psBlockName == NULL)
		{
			return stItemsList;
		}
		
		/*
		����Ʒ��ʽ����\���񷽿鴦���š�������������ע�����ֲ����⴦��\
		ǽ�ϵķ���\
		��ͬ���໨�裨�ž�id������Ҫע�⣩\��ҩ������ͬ�����ﲻͬid��\������ĵ���ת��Ϊ����+����\
		���ߵ���\������ת��Ϊˮ\���ء�����תΪ����\
		ˮ���ҽ������崦��\��ש����\
		ͬһ���ڶ����Ʒ�����ķ���ת����ѩ\���군\���ݲ�\����\ӣ���أ�\
		ͬһ���ڶ�����������ת����������������£��������磩\
		���ֲ��ֲ�ﴦ��������ֲ����ϲ������ʹ����о�����Ҷ����(��ݿ���������⴦��ֱ��������Ĭ�����̣���ݿֲ��Ҳֱ��ת��Ϊ��Ʒ���������治�ɻ�ȡ�����Ǵ������ã�)
			���⽬��ֲ�꣨��Ѩ����ת������ֲ��͸�������������ֲ��͸����������𣬲�Թ��Ҳ��ֲ��͸����������������ת����
			ע�⺣���������⣬������ˮ��ǩ�ұغ�ˮ����������ʵ�����������ֱ�Ӻ��Ժ��������Դ���ˮ�����������˻��Ƕ��һ��ˮͰ�ɣ�����ͳ��\
		����ֲ�ﴦ��С�ʹ���Ҷ���߲ݡ����ݡ����;��񻨡�ƿ�Ӳ�ֲ��\

		���ﴦ�����������ܲ�����˸���С�������Ϲϵ����ӣ������ǵ�������ʽת�������ϡ��ϹϾ��н������̬����ͨ��̬
			���⣺��ѻ�����ת������ѻ����ӣ�ƿ�Ӳ�����°벿�֣�ת����ƿ�ӲݼԹ�\

		��ͨ�����뺬ˮ���鴦����ˮ��ת��ΪˮͰ��\
		*/
		//ע�⣬ʹ���˶�·��ֵԭ��ֻҪ����һ�������ɹ����أ���ʣ��ȫ������������bRetCvrtΪtrue
		bool bRetCvrt =
			CvrtUnItemedBlocks(stBlocks, stItemsList) ||		//����Ʒ��ʽ���鴦��
			CvrtDoublePartBlocks(stBlocks, stItemsList) ||		//���񷽿鴦��
			CvrtWallVariantBlocks(stBlocks, stItemsList) ||		//ǽ�Ϸ��鴦��
			CvrtFlowerPot(stBlocks, stItemsList) ||				//���账��
			CvrtCauldron(stBlocks, stItemsList) ||				//��ҩ������
			CvrtCandleCake(stBlocks, stItemsList) ||			//���򵰸⴦��
			CvrtAliasBlocks(stBlocks, stItemsList) ||			//�������鴦��
			CvrtFluid(stBlocks, stItemsList) ||					//���崦��
			CvrtSlabBlocks(stBlocks, stItemsList) ||			//��ש����
			CvrtClusterBlocks(stBlocks, stItemsList) ||			//���Ϸ��鴦��
			CvrtPolyAttachBlocks(stBlocks, stItemsList) ||		//���渽�ŷ��鴦��
			CvrtMultiPartPlant(stBlocks, stItemsList) ||		//���ֲ�괦��
			CvrtDoublePartPlant(stBlocks, stItemsList) ||		//����ֲ�괦��
			CvrtCropPlant(stBlocks, stItemsList) ||				//����ֲ�괦��

			false;//�˴�falseֻ��Ϊ��ͳһ��ʽ���ú������ú�ͳһ��||���������ı�����ִ�н��

		//���ⷽ�顢���Ϸ��飬��񷽿飬��ˮ����ת�������е�ת��ֻ��ת������״̬���������ͺ���Ʒ����ʽһ�������������Ժ�������������ת����û��Ч��Ϊ������Ʒ
		//���ǰ��ȫ��û���봦����Ϊ��ͨ���飬ֱ�ӷ���ԭʼֵ
		//ע�����һ����Ʒ���б�ǩ�����ڹ��˱��ڣ����Դ˴��ٴδ���
		if (!bRetCvrt)
		{
			CvrtNormalBlock(stBlocks, stItemsList);
		}

		//�б�ǩ������к�ˮ���鴦���κη��鶼�п����ǣ�ȫ����һ�飩
		CvrtWaterLoggedBlock(stBlocks, stItemsList);

		return stItemsList;
	}

	//�����������entityinfoһ����Ҳ�ܼ���
	static BlockInfo BlockStatsToBlockInfo(const BlockStats &stBlocks)
	{
		return BlockInfo
		{
			stBlocks.psBlockName == NULL ? NBT_Node::NBT_String{} : *stBlocks.psBlockName,
			stBlocks.pcpdProperties == NULL ? NBT_Node::NBT_Compound{} : *stBlocks.pcpdProperties
		};
	}
private:

	/*���棬����ʹ��pcpdProperties�ĵط�����Ҫ�ж��Ƿ�ΪNULL��*/

//����ʹ�����к�ĵط������ǳ����ַ�����
#define FIND(name)\
const static std::string target = MU8STR(name);\
size_t szPos = stBlocks.psBlockName->find(target);\
if (szPos != std::string::npos)

#define STARTSWITH(name)\
const static std::string target = MU8STR(name);\
if (stBlocks.psBlockName->starts_with(target))

#define ENDSWITH(name)\
const static std::string target = MU8STR(name);\
if (stBlocks.psBlockName->ends_with(target))

	static inline bool CvrtUnItemedBlocks(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)
	{
		//ֱ��ƥ�����в��ɻ�ȡ�ķ��鲢���ؿգ�ע����mc����������Ʒ��ʽ�ģ��������治�ɻ�ȡ��
		//ע�ⲻƥ���š����������ȶ�񷽿����һ�룬�������Ƕ�Ӧ�ĺ������д���
		const static std::unordered_set<NBT_Node::NBT_String> setUnItemedBlocks =
		{
			//������
			MU8STR("minecraft:air"),
			MU8STR("minecraft:void_air"),
			MU8STR("minecraft:cave_air"),
			//��������
			MU8STR("minecraft:nether_portal"),
			MU8STR("minecraft:end_portal"),
			MU8STR("minecraft:end_gateway"),
			//����
			MU8STR("minecraft:fire"),
			MU8STR("minecraft:soul_fire"),
			//�ƶ��еĻ���
			MU8STR("minecraft:moving_piston"),
		};

		//���count���ز���0�������ڣ�ֱ�ӷ���true���ɣ�stItems��ʼ����Ϊȫ��ȫ0
		return setUnItemedBlocks.count(*stBlocks.psBlockName) != 0;
	}

	static inline bool CvrtDoublePartBlocks(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)
	{
		//��ֻ���°���䣬�ϰ�ֱ��תΪ��
		{
			ENDSWITH("_door")
			{
				//��ȡ����state�ж����ŵ���һ����
				if (stBlocks.pcpdProperties == NULL)
				{
					return false;
				}
				const auto &half = stBlocks.pcpdProperties->GetString(MU8STR("half"));

				if (half == MU8STR("lower"))
				{
					stItemsList.emplace_back(*stBlocks.psBlockName, stBlocks.u64Counter * 1);
				}
				//else if (half == MU8STR("upper")){}
				//else{}

				return true;
			}
		}

		//��ֻ��ͷ���䣬��ֱ��תΪ��
		{
			ENDSWITH("_bed")
			{
				//��ȡ����state�ж��Ǵ�����һ����
				if (stBlocks.pcpdProperties == NULL)
				{
					return false;
				}
				const auto &part = stBlocks.pcpdProperties->GetString(MU8STR("part"));

				if (part == MU8STR("head"))
				{
					stItemsList.emplace_back(*stBlocks.psBlockName, stBlocks.u64Counter * 1);
				}
				//else if (part == MU8STR("foot")){}
				//else{}

				return true;
			}
		}

		//����ֻ�����ӵ��䣬ͷֱ��תΪ�գ�ע���ƶ��еĻ������ɻ�ȡ���ѱ����������ų�
		{
			FIND("piston")
			{
				//�ж��ǲ���piston_head
				if (*stBlocks.psBlockName != MU8STR("minecraft:piston_head"))
				{
					stItemsList.emplace_back(*stBlocks.psBlockName, stBlocks.u64Counter * 1);
				}
				//else{}

				return true;
			}
		}

		return false;
	}

	//ע��ֻ����ǽ�ϵ���ʽ���������ͨ��ʽ��������Ҫ����
	static inline bool CvrtWallVariantBlocks(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)
	{
		//���������д�ǽ�϶��ⷽ��id�ķ��鶼����wall�޶��ʣ��Ҳ���wall��β
		//ȥ��wallת��Ϊitem��ʽ�������鿴������������wall_��β�ķ�������Ҳ������wall_wall_֮����������
		//ֱ��ƥ��wall_��ɾ����������ȷ��������wall_torch����cyan_wall_bannerΪ��Ʒ��ʽ

		FIND("wall_")
		{
			auto TmpName = *stBlocks.psBlockName;
			TmpName.erase(szPos, target.length());//ɾȥwall_
			stItemsList.emplace_back(std::move(TmpName), stBlocks.u64Counter * 1);

			return true;
		}

		return false;
	}

	//ע�⣬ֻ����Ź����Ļ��裬������������������flower_pot�Ǿ͸�������Ҫת����
	static inline bool CvrtFlowerPot(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)
	{
		//���˻��Ļ�����potted_��ͷ���������
		//�����flower_pot
		FIND("potted_")
		{
			//�е������ת��Ϊflower_pot+��չ����Ʒ������ʽ
			stItemsList.emplace_back(MU8STR("minecraft:flower_pot"), stBlocks.u64Counter * 1);
			auto TmpName = *stBlocks.psBlockName;
			TmpName.erase(szPos, target.length());//ɾȥpotted_
			//���ĩβ�Ƿ���_bush��β
			ENDSWITH("_bush")
			{
				//�ž黨���⴦��
				//ɾȥbush
				TmpName.erase(TmpName.length() - target.length());//ɾȥĩβ_bush
			}

			stItemsList.emplace_back(std::move(TmpName), stBlocks.u64Counter * 1);

			return true;
		}

		return false;
	}

	static inline bool CvrtCauldron(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)
	{
		//��ҩ������ˮ���ҽ�����ѩ
		//���к�ˮ���������ת������ƿ

		enum CauldronType
		{
			Cauldron = 0,
			Water_cauldron,
			Lava_cauldron,
			Powder_snow_cauldron,
		};

		const static std::unordered_map<NBT_Node::NBT_String, CauldronType> mapCauldron =
		{
			{MU8STR("minecraft:cauldron"),Cauldron},
			{MU8STR("minecraft:water_cauldron"),Water_cauldron},
			{MU8STR("minecraft:lava_cauldron"),Lava_cauldron},
			{MU8STR("minecraft:powder_snow_cauldron"),Powder_snow_cauldron},
		};

		auto it = mapCauldron.find(*stBlocks.psBlockName);
		if (it != mapCauldron.end())
		{
			switch (it->second)
			{
			case Cauldron:
				{
					stItemsList.emplace_back(MU8STR("minecraft:cauldron"), stBlocks.u64Counter * 1);
				}
				break;
			case Water_cauldron:
				{
					stItemsList.emplace_back(MU8STR("minecraft:cauldron"), stBlocks.u64Counter * 1);
					//��չ����ΪˮͰ
					stItemsList.emplace_back(MU8STR("minecraft:water_bucket"), stBlocks.u64Counter * 1);
					//�жϷ����ǩ
					if (stBlocks.pcpdProperties == NULL)
					{
						return false;
					}
					const auto &level = stBlocks.pcpdProperties->GetString(MU8STR("level"));

					if (level == MU8STR("1"))
					{
						stItemsList.emplace_back(MU8STR("minecraft:glass_bottle"), stBlocks.u64Counter * 2);//2����ƿ�Ƴ�ˮ��ʣ��1
					}
					else if (level == MU8STR("2"))
					{
						stItemsList.emplace_back(MU8STR("minecraft:glass_bottle"), stBlocks.u64Counter * 1);//1����ƿ�Ƴ�ˮ��ʣ��2
					}
					//else if (level == MU8STR("3")){}//0����ƿ�Ƴ�ˮ��ʣ��3
					//else{}
				}
				break;
			case Lava_cauldron:
				{
					stItemsList.emplace_back(MU8STR("minecraft:cauldron"), stBlocks.u64Counter * 1);
					//��չ����Ϊ�ҽ�Ͱ
					stItemsList.emplace_back(MU8STR("minecraft:lava_bucket"), stBlocks.u64Counter * 1);
				}
				break;
			case Powder_snow_cauldron:
				{
					stItemsList.emplace_back(MU8STR("minecraft:cauldron"), stBlocks.u64Counter * 1);
					//��չ����Ϊ��ѩͰ
					stItemsList.emplace_back(MU8STR("minecraft:powder_snow_bucket"), stBlocks.u64Counter * 1);
				}
				break;
			default:
				return false;
				break;
			}

			return true;
		}

		return false;
	}

	static inline bool CvrtCandleCake(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)
	{
		ENDSWITH("_cake")
		{
			//ת��Ϊ����+����
			stItemsList.emplace_back(MU8STR("minecraft:cake"), stBlocks.u64Counter * 1);
			auto TmpName = *stBlocks.psBlockName;
			TmpName.erase(TmpName.length() - target.length());//ɾȥ_cake
			stItemsList.emplace_back(std::move(TmpName), stBlocks.u64Counter * 1);

			return true;
		}

		return false;
	}

	static inline bool CvrtAliasBlocks(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)//��������
	{
		//ӳ��
		const static std::unordered_map<NBT_Node::NBT_String, NBT_Node::NBT_String> mapAliasBlocks =
		{
			{MU8STR("minecraft:tripwire"),MU8STR("minecraft:string")},
			{MU8STR("minecraft:redstone_wire"),MU8STR("minecraft:redstone")},
			{MU8STR("minecraft:bubble_column"),MU8STR("minecraft:water_bucket")},
			//{MU8STR("minecraft:farmland"),MU8STR("minecraft:dirt")},
			//{MU8STR("minecraft:dirt_path"),MU8STR("minecraft:dirt")},
		};

		//����
		const auto it = mapAliasBlocks.find(*stBlocks.psBlockName);
		if (it != mapAliasBlocks.end())//����
		{
			stItemsList.emplace_back(it->second, stBlocks.u64Counter * 1);//����ӳ��
			return true;
		}

		return false;
	}

	static inline bool CvrtFluid(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)//����
	{
		if (*stBlocks.psBlockName == MU8STR("minecraft:water"))
		{
			if (stBlocks.pcpdProperties == NULL)
			{
				return false;
			}
			if (stBlocks.pcpdProperties->GetString(MU8STR("level")) == MU8STR("0"))//��0����ˮԴ
			{
				stItemsList.emplace_back(MU8STR("minecraft:water_bucket"), stBlocks.u64Counter * 1);
			}

			return true;
		}
		else if (*stBlocks.psBlockName == MU8STR("minecraft:lava"))
		{
			if (stBlocks.pcpdProperties == NULL)
			{
				return false;
			}
			if (stBlocks.pcpdProperties->GetString(MU8STR("level")) == MU8STR("0"))//��0�����ҽ�Դ
			{
				stItemsList.emplace_back(MU8STR("minecraft:lava_bucket"), stBlocks.u64Counter * 1);
			}

			return true;
		}
		//else{}

		return false;
	}

	static inline bool CvrtSlabBlocks(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)
	{
		ENDSWITH("_slab")
		{
			if (stBlocks.pcpdProperties == NULL)
			{
				return false;
			}
			if (stBlocks.pcpdProperties->GetString(MU8STR("type")) == MU8STR("double"))//ת��Ϊ2������Ϊ1
			{
				stItemsList.emplace_back(*stBlocks.psBlockName, stBlocks.u64Counter * 2);
			}
			else
			{
				stItemsList.emplace_back(*stBlocks.psBlockName, stBlocks.u64Counter * 1);
			}

			return true;
		}

		return false;
	}

	static inline bool CvrtClusterBlocks(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)
	{
#define EMPLACE_CLUSTER_ITEMS(name)\
const auto &##name = stBlocks.pcpdProperties->GetString(MU8STR(#name));\
stItemsList.emplace_back(*stBlocks.psBlockName, stBlocks.u64Counter * std::stoll(##name));

		//���жϴ治���ڸ���״̬���������д���·������ʹ�ã������ڿ�ͷ�ų�
		if (stBlocks.pcpdProperties == NULL)
		{
			return false;
		}

		//�������ȴ�������ɫ���飩
		ENDSWITH("candle")//���������⣬��������ɫ��ǩ�İ汾���봲֮�಻ͬ��
		{
			//��Ϊǰ���Ѿ������������ĵ��⣬�ų��Ǵ����򵰸���������candle�ǽ�β�������ڵ�������
			EMPLACE_CLUSTER_ITEMS(candles);//ת�����������������
			return true;
		}


		enum ClusterBlock :uint64_t
		{
			Snow = 0, 
			Turtle_egg,
			Sea_pickle,
			Pink_petals,
		};

		const static std::unordered_map<NBT_Node::NBT_String, ClusterBlock> mapClusterBlocks =
		{
			{MU8STR("minecraft:snow"),Snow},
			{MU8STR("minecraft:turtle_egg"),Turtle_egg},
			{MU8STR("minecraft:sea_pickle"),Sea_pickle},
			{MU8STR("minecraft:pink_petals"),Pink_petals},
		};

		const auto it = mapClusterBlocks.find(*stBlocks.psBlockName);
		if (it != mapClusterBlocks.end())
		{

			switch (it->second)
			{
			case Snow://layers str
				{
					EMPLACE_CLUSTER_ITEMS(layers);
				}
				break;
			case Turtle_egg://eggs str
				{
					EMPLACE_CLUSTER_ITEMS(eggs);
				}
				break;
			case Sea_pickle://pickles str
				{
					EMPLACE_CLUSTER_ITEMS(pickles);
				}
				break;
			case Pink_petals://flower_amount str
				{
					EMPLACE_CLUSTER_ITEMS(flower_amount);
				}
				break;
			default:
				return false;
				break;
			}

			return true;
		}

		return false;
#undef EMPLACE_CLUSTER_ITEMS
	}

	static inline bool CvrtPolyAttachBlocks(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)//���渽�ŷ���
	{
		const static std::unordered_set<NBT_Node::NBT_String> setPolyAttachBlocks =
		{
			MU8STR("minecraft:vine"),
			MU8STR("minecraft:glow_lichen"),
			MU8STR("minecraft:sculk_vein"),
		};

		const constexpr static char *pSurfaceNameList[] =
		{
			"down",
			"east",
			"north",
			"south",
			"up",
			"west",
		};


		if (setPolyAttachBlocks.count(*stBlocks.psBlockName) != 0)
		{
			if (stBlocks.pcpdProperties == NULL)
			{
				return false;
			}

			//�������б���ѯÿ������Ϣ��������ڣ��ж��ǲ���true������ǣ�����+1
			uint64_t u64Counter = 0;
			for (const auto &it : pSurfaceNameList)
			{
				const auto pSurface = stBlocks.pcpdProperties->HasString(MU8STR(it));
				if (pSurface != NULL)
				{
					if (*pSurface == MU8STR("true"))
					{
						++u64Counter;
					}
				}
			}

			//���ݼ��������Ӧ�����Ķ��渽�ſ���Ʒ��
			stItemsList.emplace_back(*stBlocks.psBlockName, stBlocks.u64Counter * u64Counter);

			return true;
		}

		return false;
	}


	//���д���ת����������������ͨ����ֲ��
	static inline bool CvrtMultiPartPlant(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)//���ֲ�괦��
	{
		//ֻת��ֲ�������ֲ������ͨ����normal����
		const static std::unordered_map<NBT_Node::NBT_String, NBT_Node::NBT_String> mapMultiPartPlant =
		{
			//��Ѩ�������⴦��ת�������⽬��
			{MU8STR("minecraft:cave_vines_plant"),MU8STR("minecraft:glow_berries")},
			{MU8STR("minecraft:cave_vines"),MU8STR("minecraft:glow_berries")},
			//��������ת��
			{MU8STR("minecraft:bamboo_sapling"),MU8STR("minecraft:bamboo")},
			//����Ҷ����ת��
			{MU8STR(" minecraft:big_dripleaf_stem"),MU8STR("minecraft:big_dripleaf")},
			//����
			{MU8STR("minecraft:kelp"),MU8STR("minecraft:kelp")},//��Ӻ����������ӳ���Ŀ����Ϊ�˴���ˮ���
			{MU8STR("minecraft:kelp_plant"),MU8STR("minecraft:kelp")},
			//����
			{MU8STR("minecraft:tall_seagrass"),MU8STR("minecraft:seagrass")},//�ߺ��ݵ�ÿ�����ֶ�ӳ�䵽һ������
			//�½�����
			{MU8STR("minecraft:weeping_vines_plant"),MU8STR("minecraft:weeping_vines")},
			{MU8STR("minecraft:twisting_vines_plant"),MU8STR("minecraft:twisting_vines")},
		};

		const auto it = mapMultiPartPlant.find(*stBlocks.psBlockName);
		if (it != mapMultiPartPlant.end())
		{
			stItemsList.emplace_back(it->second, stBlocks.u64Counter * 1);//����ӳ��
			//�������ݱغ�ˮ��ǿ�ƴ���
			if (it->second == MU8STR("minecraft:kelp")||
				it->second == MU8STR("minecraft:seagrass"))
			{
				stItemsList.emplace_back(MU8STR("minecraft:water_bucket"), stBlocks.u64Counter * 1);
			}
			return true;
		}
		
		return false;
	}

	//���д���ת����������������ͨ����ֲ��
	static inline bool CvrtDoublePartPlant(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)//˫��ֲ�괦��
	{
		//ֻ�и����ƻ����䣬ͷ��ֱ�Ӻ���
		const static std::unordered_set<NBT_Node::NBT_String> setDoublePartPlant =
		{
			MU8STR("minecraft:small_dripleaf"),
			MU8STR("minecraft:tall_grass"),
			MU8STR("minecraft:large_fern"),
			MU8STR("minecraft:sunflower"),
			MU8STR("minecraft:lilac"),
			MU8STR("minecraft:rose_bush"),
			MU8STR("minecraft:peony"),
			MU8STR("minecraft:pitcher_plant"),
		};

		//�ж��Ƿ������set�У����������²���
		if (setDoublePartPlant.count(*stBlocks.psBlockName) != 0)
		{
			if (stBlocks.pcpdProperties == NULL)
			{
				return false;
			}
			const auto &half = stBlocks.pcpdProperties->GetString("half");
			if (half == MU8STR("lower"))
			{
				stItemsList.emplace_back(*stBlocks.psBlockName, stBlocks.u64Counter * 1);
			}
			//else if (half == MU8STR("upper")) {}
			//else {}

			return true;
		}

		return false;
	}
	
	//���д���ת����������������ͨ����ֲ��
	static inline bool CvrtCropPlant(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)//����ֲ�괦��
	{
		const static std::unordered_map<NBT_Node::NBT_String, NBT_Node::NBT_String> mapCropPlant =
		{
			{MU8STR("minecraft:pumpkin_stem"),MU8STR("minecraft:pumpkin_seeds")},
			{MU8STR("minecraft:attached_pumpkin_stem"),MU8STR("minecraft:pumpkin_seeds")},
			{MU8STR("minecraft:melon_stem"),MU8STR("minecraft:melon_seeds")},
			{MU8STR("minecraft:attached_melon_stem"),MU8STR("minecraft:melon_seeds")},
			{MU8STR("minecraft:beetroots"),MU8STR("minecraft:beetroot_seeds")},
			{MU8STR("minecraft:wheat"),MU8STR("minecraft:wheat_seeds")},
			{MU8STR("minecraft:carrots"),MU8STR("minecraft:carrot")},
			{MU8STR("minecraft:potatoes"),MU8STR("minecraft:potato")},
			{MU8STR("minecraft:torchflower_crop"),MU8STR("minecraft:torchflower_seeds")},
			{MU8STR("minecraft:pitcher_crop"),MU8STR("minecraft:pitcher_pod")},//��Ҫ���д���
		};

		const auto it = mapCropPlant.find(*stBlocks.psBlockName);
		if (it != mapCropPlant.end())
		{
			if (it->second == MU8STR("minecraft:pitcher_pod"))//��Ҫ�ж�ԭ�����half
			{
				if (stBlocks.pcpdProperties == NULL)
				{
					return false;
				}
				const auto &half = stBlocks.pcpdProperties->GetString("half");
				if (half == MU8STR("lower"))
				{
					stItemsList.emplace_back(it->second, stBlocks.u64Counter * 1);
				}
				//else if (half == MU8STR("upper")) {}
				//else {}
			}
			else
			{
				//ֱ�Ӳ���ת�����
				stItemsList.emplace_back(it->second, stBlocks.u64Counter * 1);
			}

			return true;
		}

		return false;
	}

	//ע��ú�����Ҫ��֤�������κ�block��compound�ж�
	static inline void CvrtNormalBlock(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)
	{
		stItemsList.emplace_back(*stBlocks.psBlockName, stBlocks.u64Counter * 1);
	}

	static inline void CvrtWaterLoggedBlock(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)
	{
		//�ж��ǲ��Ǻ�ˮ���飬����ǣ���һ��ˮͰ
		if (stBlocks.pcpdProperties == NULL)
		{
			return;//���û�и��ӱ�ǩ�������ж�
		}
		const auto it = stBlocks.pcpdProperties->HasString("waterlogged");
		if (it != NULL)
		{
			if (*it == MU8STR("true"))//��ˮ
			{
				stItemsList.emplace_back(MU8STR("minecraft:water_bucket"), stBlocks.u64Counter * 1);
			}
			//else {}
		}
	}

#undef ENDSWITH
#undef STARTSWITH
#undef FIND
	//class
};