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
		const auto &Regions = nRoot.Compound().at(MU8STR("Regions")).Compound();
		//��������ͳ��vector
		std::vector<RegionsStatistics> vtRegionsStatistics;
		vtRegionsStatistics.reserve(Regions.size());//��ǰ����
		for (const auto &[RgName, RgVal] : Regions)//����ѡ��
		{
			const auto &RgCompound = RgVal.Compound();

			/*----------------�����С���㡢��ɫ���ȡ----------------*/
			//��ȡ����ƫ��
			const auto &Position = RgCompound.at(MU8STR("Position")).Compound();
			const BlockPos reginoPos =
			{
				.x = Position.at(MU8STR("x")).Int(),
				.y = Position.at(MU8STR("y")).Int(),
				.z = Position.at(MU8STR("z")).Int(),
			};
			//��ȡ�����С������Ϊ�����������ƫ�Ƽ���ʵ�ʴ�С��
			const auto &Size = RgCompound.at(MU8STR("Size")).Compound();
			const BlockPos regionSize =
			{
				.x = Size.at(MU8STR("x")).Int(),
				.y = Size.at(MU8STR("y")).Int(),
				.z = Size.at(MU8STR("z")).Int(),
			};
			//��������ʵ�ʴ�С
			const BlockPos posEndRel = getRelativeEndPositionFromAreaSize(regionSize).add(reginoPos);
			const BlockPos posMin = getMinCorner(reginoPos, posEndRel);
			const BlockPos posMax = getMaxCorner(reginoPos, posEndRel);
			const BlockPos size = posMax.sub(posMin).add({ 1,1,1 });

			//printf("RegionSize: [%d, %d, %d]\n", size.x, size.y, size.z);

			//��ȡ��ɫ�壨�������ࣩ
			const auto &BlockStatePalette = RgCompound.at(MU8STR("BlockStatePalette")).List();
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
				const auto &itCompound = it.Compound();

				BlockStatistics bsTemp{};
				bsTemp.sBlockName = itCompound.at(MU8STR("Name")).String();
				const auto find = itCompound.find(MU8STR("Properties"));//��鷽���Ƿ��ж�������
				if (find != itCompound.end())
				{
					bsTemp.cpdProperties = (*find).second.Compound();

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
			const auto &BlockStates = RgCompound.at(MU8STR("BlockStates")).Long_Array();
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
		NBT_Node::NBT_String sItemNameEx{};//��Ʒ����չ(Ӧ�Ի��衢��ˮ���顢��ҩ�������򵰸�������Ʒ)
		uint64_t u64Counter = 0;//��Ʒ������
		uint64_t u64CounterEx = 0;//��Ʒ��������չ(Ӧ�Ի��衢��ˮ���顢��ҩ�������򵰸�������Ʒ)
	};

	static ItemStack BlockStatisticsToItemStack(const BlockStatistics &stBlocks)
	{
		//�����鵽��Ʒת��
		ItemStack stItems;

		//static const std::map<NBT_Node::NBT_String, uint64_t> mapFilter =//uint64_t->ID
		//{
		//	/*0 -> minecraft:empty*/
		//	{MU8STR("minecraft:air"),0 },//����air��Ӧ��Ʒempty
		//	{MU8STR("minecraft:void_air"),0 },
		//	{MU8STR("minecraft:cave_air"),0 },
		//
		//	{MU8STR("minecraft:piston_head"),0},
		//	//{MU8STR("minecraft:moving_piston"),0},//�ƶ��еĻ�����Ҫ����
		//	{MU8STR("minecraft:nether_portal"),0},
		//	{MU8STR("minecraft:end_portal"),0},
		//	{MU8STR("minecraft:end_gateway"),0},
		//
		//	{MU8STR("minecraft:farmland"),1},//dirt
		//
		//	{MU8STR("minecraft:lava"),2},//lava.level == 0 ? minecraft:lava_bucket : minecraft:empty
		//	{MU8STR("minecraft:water"),2},//water.level == 0 ? minecraft:water_bucket : minecraft:empty
		//};






		//���ⷽ�顢���Ϸ��飬��񷽿飬��ˮ����ת�������е�ת��ֻ��ת������״̬���������ͺ���Ʒ����ʽһ�������������Ժ�������������ת����û��Ч��Ϊ������Ʒ

		//ע�⣬ʹ���˶�·��ֵԭ��ֻҪ����һ�������ɹ����أ���ʣ��ȫ������������bRetCvrtΪtrue
		bool bRetCvrt =
		/*
		��\��\��ֲ�С�ʹ���Ҷ���߲���ͬ����������ֲ����ϲ������ʹ����о�����Ҷ�������⽬��ֲ�꣨��Ѩ���������ϲ����²�������
		\��ͬ���໨��\ǽ�ϵķ���\��ҩ��\����\������ת��Ϊˮ\������ĵ���ת��Ϊ����+����\
		*/
			CurtUnItemedBlocks(stBlocks, stItems) ||
			CvrtWallVariantBlocks(stBlocks, stItems) ||
			CvrtFlowerPot(stBlocks, stItems)
			;//TODO





		//��������ת��
		/*
		��ש\ѩ\���군\���ݲ�\����\ӣ����
		*/

		//���ⷽ������ת��
		/*
		�����������飬��������������£���������
		*/


		//���ǰ��ȫ��û���봦����Ϊ��ͨ���飬ֱ�ӷ���ԭʼֵ
		if (!bRetCvrt)
		{
			stItems.sItemName = stBlocks.sBlockName;
			stItems.u64Counter = 1;
		}

		//��ˮ������Ҫ���⴦���κη��鶼�п����Ǻ�ˮ��һ��blockstate�鿴��������ת��ΪˮͰ����Ex��չ

		return stItems;
	}


	static inline bool CurtUnItemedBlocks(const BlockStatistics &stBlocks, ItemStack &stItems)
	{
		//ֱ��ƥ�����в��ɻ�ȡ�ķ��鲢���ؿգ�ע����mc����������Ʒ��ʽ�ģ��������治�ɻ�ȡ��
		//ע�ⲻƥ���š����ȶ�񷽿����һ�룬�������Ƕ�Ӧ�ĺ������д���
		std::unordered_set<NBT_Node::NBT_String> UnItemedBlocks =
		{
			MU8STR("minecraft:air"),
			MU8STR("minecraft:void_air"),
			MU8STR("minecraft:cave_air"),
			MU8STR("minecraft:piston_head"),
			MU8STR("minecraft:nether_portal"),
			MU8STR("minecraft:end_portal"),
			MU8STR("minecraft:end_gateway"),
			MU8STR("minecraft:fire"),
			MU8STR("minecraft:minecraft:soul_fire"),
			MU8STR("minecraft:"),
			MU8STR("minecraft:"),
			MU8STR("minecraft:"),
			MU8STR("minecraft:"),
		};

		//���count���ز���0�������ڣ�ֱ�ӷ���true���ɣ�stItems��ʼ����Ϊȫ��ȫ0
		return UnItemedBlocks.count(stBlocks.sBlockName) !=0;
	}


	//ע��ֻ����ǽ�ϵ���ʽ���������ͨ��ʽ��������Ҫ����
	static inline bool CvrtWallVariantBlocks(const BlockStatistics &stBlocks, ItemStack &stItems)
	{
		//���������д�ǽ�϶��ⷽ��id�ķ��鶼����wall�޶��ʣ��Ҳ���wall��β
		//ȥ��wallת��Ϊitem��ʽ�������鿴������������wall_��β�ķ�������Ҳ������wall_wall_֮����������
		//ֱ��ƥ��wall_��ɾ����������ȷ��������wall_torch����cyan_wall_bannerΪ��Ʒ��ʽ

		const std::string target = MU8STR("wall_");

		size_t szPos = stBlocks.sBlockName.find(target);
		if (szPos != std::string::npos)
		{
			stItems.sItemName = stBlocks.sBlockName;
			stItems.sItemName.replace(szPos, target.length(), "");//ɾȥwall_
			stItems.u64Counter = 1;//1����Ʒ
			return true;
		}

		return false;
	}

	//ע�⣬ֻ����Ź����Ļ��裬������������������flower_pot�Ǿ͸�������Ҫת����
	static inline bool CvrtFlowerPot(const BlockStatistics &stBlocks, ItemStack &stItems)
	{
		//���˻��Ļ�����potted_��ͷ���������
		//�����flower_pot

		const std::string target = MU8STR("potted_");
		size_t szPos = stBlocks.sBlockName.find(target);
		if (szPos != std::string::npos)
		{
			//�е������ת��Ϊflower_pot+��չ����Ʒ������ʽ
			stItems.sItemName = "minecraft:flower_pot";
			stItems.u64Counter = 1;

			stItems.sItemNameEx = stBlocks.sBlockName.substr(szPos + target.length());
			stItems.u64CounterEx = 1;

			return true;
		}

		return false;
	}


};