#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"
#include "Calc_Tool.hpp"

#include <stdint.h>
#include <vector>
#include <map>
#include <regex>
#include <unordered_set>

class BlockProcess
{
private:



public:
	BlockProcess() = delete;
	~BlockProcess() = delete;

	struct BlockStatistics
	{
		NBT_Node::NBT_String sBlockName{};
		NBT_Node::NBT_Compound cpdProperties{};
		uint64_t u64Counter = 0;//���������
	};

	struct RegionsStatistics
	{
		NBT_Node::NBT_String sRegionName{};
		std::vector<BlockStatistics> bsList{};
	};

	static std::vector<RegionsStatistics> GetBlockStatistics(const NBT_Node &nRoot)//block list
	{
		//��ȡregions��Ҳ��������һ��ͶӰ�����ж������ѡ����
		const auto &Regions = nRoot.Compound(MU8STR("Regions"));
		//��������ͳ��vector
		std::vector<RegionsStatistics> vtRegionsStatistics;
		vtRegionsStatistics.reserve(Regions.size());//��ǰ����
		for (const auto &[RgName, RgVal] : Regions)//����ѡ��
		{

			/*----------------�����С���㡢��ɫ���ȡ----------------*/
			//��ȡ����ƫ��
			const auto &Position = RgVal.Get(MU8STR("Position"));
			const BlockPos reginoPos =
			{
				.x = Position.Int(MU8STR("x")),
				.y = Position.Int(MU8STR("y")),
				.z = Position.Int(MU8STR("z")),
			};
			//��ȡ�����С������Ϊ�����������ƫ�Ƽ���ʵ�ʴ�С��
			const auto &Size = RgVal.Get(MU8STR("Size"));
			const BlockPos regionSize =
			{
				.x = Size.Int(MU8STR("x")),
				.y = Size.Int(MU8STR("y")),
				.z = Size.Int(MU8STR("z")),
			};
			//��������ʵ�ʴ�С
			const BlockPos posEndRel = getRelativeEndPositionFromAreaSize(regionSize).add(reginoPos);
			const BlockPos posMin = getMinCorner(reginoPos, posEndRel);
			const BlockPos posMax = getMaxCorner(reginoPos, posEndRel);
			const BlockPos size = posMax.sub(posMin).add({ 1,1,1 });

			//printf("RegionSize: [%d, %d, %d]\n", size.x, size.y, size.z);

			//��ȡ��ɫ�壨�������ࣩ
			const auto &BlockStatePalette = RgVal.List(MU8STR("BlockStatePalette"));
			const uint32_t bitsPerBitMapElement = Max(2U, (uint32_t)sizeof(uint32_t) * 8 - numberOfLeadingZeros(BlockStatePalette.size() - 1));//����λͼ��һ��Ԫ��ռ�õ�bit��С
			const uint32_t bitMaskOfElement = (1 << bitsPerBitMapElement) - 1;//��ȡ����λ������ȡbitmap�ڲ�����
			//printf("BlockStatePaletteSize: [%zu]\nbitsPerBitMapElement: [%d]\n", BlockStatePalette.size(), bitsPerBitMapElement);
			/*------------------------------------------------*/

			/*----------------���ݷ���λͼ���ʵ�ɫ�壬��ȡ��ͬ״̬����ļ���----------------*/
			//��������ͳ��vector
			std::vector<BlockStatistics> vtBlockStatistics;
			vtBlockStatistics.reserve(BlockStatePalette.size());//��ǰ����

			//����BlockStatePalette�����ɫ�壬�����д�����Ч�±�ķ���ͳ��vector
			for (const auto &it : BlockStatePalette)
			{
				BlockStatistics bsTemp{};
				bsTemp.sBlockName =it.String(MU8STR("Name"));
				const auto find = it.HasCompound(MU8STR("Properties"));//��鷽���Ƿ��ж�������
				if (find != NULL)
				{
					bsTemp.cpdProperties = *find;

					/*
					blockName += '[';
					const auto &Properties = (*find).second.Compound();//ȡ��compound
					for (const auto &[ppState, ppVal] : Properties)//�������ж������Բ�ƴ�ӵ�blockName��
					{
						blockName += ppState;
						blockName += '=';
						blockName += ppVal.String();
						blockName += ',';
					}

					//������ǿ��б��滻���һ��,Ϊ]
					if (blockName.ends_with(','))
					{
						blockName.back() = ']';
					}
					else
					{
						blockName += ']';
					}
					*/
				}

				vtBlockStatistics.emplace_back(std::move(bsTemp));
			}

			//��ȡLong����״̬λͼ���飨������Ϊ�±���ʵ�ɫ�壩
			const auto &BlockStates = RgVal.Long_Array(MU8STR("BlockStates"));
			const uint64_t RegionFullSize = (uint64_t)size.x * (uint64_t)size.y * (uint64_t)size.z;
			if (BlockStates.size() * 64 / bitsPerBitMapElement < RegionFullSize)
			{
				//printf("BlockStates Too Small!\n");
				vtRegionsStatistics.emplace_back(RgName,std::move(vtBlockStatistics));//ȫ0����
				continue;//���Ի�ȡ��һ��ѡ��
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

				++vtBlockStatistics[paletteIndex].u64Counter;
			}
			/*------------------------------------------------*/

			//������ɣ�����RegionsStatistics
			vtRegionsStatistics.emplace_back(RgName,std::move(vtBlockStatistics));
		}

		return vtRegionsStatistics;
	}

	//���˿ɻ�ȡ�ķ��飬���ɻ�ȡ�ķ���ֱ��ɾ������ש���������Ʒ����ת������
	struct ItemStack
	{
		NBT_Node::NBT_String sItemName{};//��Ʒ��
		uint64_t u64Counter = 0;//��Ʒ������
	};

	using ItemStackList = std::vector<ItemStack>;//(Ӧ�Ի��衢��ˮ���顢��ҩ�������򵰸�������Ʒ)

	static ItemStackList BlockStatisticsToItemStack(const BlockStatistics &stBlocks)
	{
		//�����鵽��Ʒת��
		ItemStackList stItemsList;

		//	{MU8STR("minecraft:farmland"),1},//dirt
		//
		//	{MU8STR("minecraft:lava"),2},//lava.level == 0 ? minecraft:lava_bucket : minecraft:empty
		//	{MU8STR("minecraft:water"),2},//water.level == 0 ? minecraft:water_bucket : minecraft:empty

		
		/*
		����Ʒ��ʽ����\��񷽿鴦���š�������������ע�����ֲ����⴦��\
		ǽ�ϵķ���\
		��ͬ���໨��\��ҩ������ͬ�����ﲻͬid��\������ĵ���ת��Ϊ����+����\
		���ߵ���\������ת��Ϊˮ\���ء�����תΪ����\
		ˮ���ҽ������崦��\��ˮ���鴦��ת��ΪˮͰ��\
		����ֲ�������ֲ����ϲ������ʹ����о�����Ҷ������ݿ����ֲ���ͷ�������⴦��ֲ����Ʒ��ʽҲ��plant��׺����
			���⽬��ֲ�꣨��Ѩ����ת������ֲ��͸�������������ֲ��͸����������𣬲�Թ��Ҳ��ֲ��͸�����
		ͬһ���ڶ����Ʒ�����ķ���ת������ש\ѩ\���군\���ݲ�\����\ӣ���أ�\
		����ֲ�ƿ�ӲݼԹ���ֲ�꣬��ѻ����Ӻ�ֲ�꣩С�ʹ���Ҷ���߲ݡ����;��񻨡�ƿ�Ӳ�ֲ�꣨���⴦����Ʒ��ʽҲ��plant��׺��\
		ͬһ���ڶ�����������ת����������������£��������磩
		*/
		//ע�⣬ʹ���˶�·��ֵԭ��ֻҪ����һ�������ɹ����أ���ʣ��ȫ������������bRetCvrtΪtrue
		bool bRetCvrt =
			CvrtUnItemedBlocks(stBlocks, stItemsList) ||//����Ʒ��ʽ���鴦��
			CvrtMultiBlock(stBlocks, stItemsList) || //��񷽿鴦��
			CvrtWallVariantBlocks(stBlocks, stItemsList) ||//ǽ�Ϸ��鴦��
			CvrtFlowerPot(stBlocks, stItemsList) ||//���账��



			false;//�˴�falseֻ��Ϊ��ͳһ��ʽ���ú������ú�ͳһ��||���������ı�����ִ�н��

		//���ⷽ�顢���Ϸ��飬��񷽿飬��ˮ����ת�������е�ת��ֻ��ת������״̬���������ͺ���Ʒ����ʽһ�������������Ժ�������������ת����û��Ч��Ϊ������Ʒ
		//���ǰ��ȫ��û���봦����Ϊ��ͨ���飬ֱ�ӷ���ԭʼֵ
		if (!bRetCvrt)
		{
			stItemsList.emplace_back(stBlocks.sBlockName, 1);
		}

		return stItemsList;
	}

private:

#define FIND(name)\
const std::string target = MU8STR(name);\
size_t szPos = stBlocks.sBlockName.find(target);\
if (szPos != std::string::npos)

	static inline bool CvrtUnItemedBlocks(const BlockStatistics &stBlocks, ItemStackList &stItemsList)
	{
		//ֱ��ƥ�����в��ɻ�ȡ�ķ��鲢���ؿգ�ע����mc����������Ʒ��ʽ�ģ��������治�ɻ�ȡ��
		//ע�ⲻƥ���š����������ȶ�񷽿����һ�룬�������Ƕ�Ӧ�ĺ������д���
		std::unordered_set<NBT_Node::NBT_String> UnItemedBlocks =
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
			MU8STR("minecraft:minecraft:soul_fire"),
			//�ƶ��еĻ���
			MU8STR("minecraft:moving_piston"),
		};

		//���count���ز���0�������ڣ�ֱ�ӷ���true���ɣ�stItems��ʼ����Ϊȫ��ȫ0
		return UnItemedBlocks.count(stBlocks.sBlockName) != 0;
	}

	static inline bool CvrtMultiBlock(const BlockStatistics &stBlocks, ItemStackList &stItemsList)
	{
		//��ֻ���°���䣬�ϰ�ֱ��תΪ��
		{
			FIND("_door")
			{
				//��ȡ����state�ж����ŵ���һ����
				const auto &half = stBlocks.cpdProperties.at(MU8STR("half")).String();

				if (half == MU8STR("lower"))
				{
					stItemsList.emplace_back(stBlocks.sBlockName, 1);
				}
				//else if (half == MU8STR("upper")){}
				//else{}

				return true;
			}
		}

		//��ֻ��ͷ���䣬��ֱ��תΪ��
		{
			FIND("_bed")
			{
				//��ȡ����state�ж��Ǵ�����һ����
				const auto &part = stBlocks.cpdProperties.at(MU8STR("part")).String();

				if (part == MU8STR("head"))
				{
					stItemsList.emplace_back(stBlocks.sBlockName, 1);
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
				if (stBlocks.sBlockName != MU8STR("minecraft:piston_head"))
				{
					stItemsList.emplace_back(stBlocks.sBlockName, 1);
				}
				//else{}

				return true;
			}
		}

		return false;
	}

	//ע��ֻ����ǽ�ϵ���ʽ���������ͨ��ʽ��������Ҫ����
	static inline bool CvrtWallVariantBlocks(const BlockStatistics &stBlocks, ItemStackList &stItemsList)
	{
		//���������д�ǽ�϶��ⷽ��id�ķ��鶼����wall�޶��ʣ��Ҳ���wall��β
		//ȥ��wallת��Ϊitem��ʽ�������鿴������������wall_��β�ķ�������Ҳ������wall_wall_֮����������
		//ֱ��ƥ��wall_��ɾ����������ȷ��������wall_torch����cyan_wall_bannerΪ��Ʒ��ʽ

		FIND("wall_")
		{
			auto TmpName = stBlocks.sBlockName;
			TmpName.replace(szPos, target.length(), "");//ɾȥwall_
			stItemsList.emplace_back(std::move(TmpName), 1);
			return true;
		}

		return false;
	}

	//ע�⣬ֻ����Ź����Ļ��裬������������������flower_pot�Ǿ͸�������Ҫת����
	static inline bool CvrtFlowerPot(const BlockStatistics &stBlocks, ItemStackList &stItemsList)
	{
		//���˻��Ļ�����potted_��ͷ���������
		//�����flower_pot
		FIND("potted_")
		{
			//�е������ת��Ϊflower_pot+��չ����Ʒ������ʽ
			stItemsList.emplace_back(MU8STR("minecraft:flower_pot"), 1);
			auto TmpName = stBlocks.sBlockName;
			TmpName.replace(szPos, target.length(), "");//ɾȥpotted_
			stItemsList.emplace_back(std::move(TmpName), 1);

			return true;
		}

		return false;
	}

	static inline bool CvrtCauldron(const BlockStatistics &stBlocks, ItemStackList &stItemsList)
	{
		//��ҩ������ˮ���ҽ�����ѩ
		//���к�ˮ���������ת����ˮƿ
		FIND("cauldron")
		{
			if (stBlocks.sBlockName == MU8STR("minecraft:cauldron"))
			{
				stItemsList.emplace_back(MU8STR("minecraft:cauldron"), 1);
			}
			else if (stBlocks.sBlockName == MU8STR("minecraft:water_cauldron"))
			{
				stItemsList.emplace_back(MU8STR("minecraft:cauldron"), 1);
				

			}
			else if (stBlocks.sBlockName == MU8STR("minecraft:lava_cauldron"))
			{
				stItemsList.emplace_back(MU8STR("minecraft:cauldron"), 1);
				//��չ����Ϊ�ҽ�Ͱ
				stItemsList.emplace_back(MU8STR("minecraft:lava_bucket"), 1);
			}
			else if (stBlocks.sBlockName == MU8STR("minecraft:powder_snow_cauldron"))
			{
				stItemsList.emplace_back(MU8STR("minecraft:cauldron"), 1);
				//��չ����Ϊ��ѩͰ
				stItemsList.emplace_back(MU8STR("minecraft:powder_snow_bucket"), 1);
			}
			//else{}

			return true;
		}

		return false;
	}


#undef FIND
	//class
};