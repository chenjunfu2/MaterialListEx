#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"
#include "Calc_Tool.hpp"

#include <stdint.h>
#include <vector>
#include <map>

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
		uint64_t u64Counter = 0;//方块计数器
	};

	struct RegionsStatistics
	{
		NBT_Node::NBT_String sRegionName{};
		std::vector<BlockStatistics> bsList{};
	};

	static std::vector<RegionsStatistics> GetBlockStatistics(NBT_Node nRoot)//block list
	{
		//获取regions，也就是区域，一个投影可能有多个区域（选区）
		const auto &Regions = nRoot.Compound().at(MU8STR("Regions")).Compound();
		//创建区域统计vector
		std::vector<RegionsStatistics> vtRegionsStatistics;
		vtRegionsStatistics.reserve(Regions.size());//提前分配
		for (const auto &[RgName, RgVal] : Regions)//遍历选区
		{
			const auto &RgCompound = RgVal.Compound();

			/*----------------区域大小计算、调色板获取----------------*/
			//获取区域偏移
			const auto &Position = RgCompound.at(MU8STR("Position")).Compound();
			const BlockPos reginoPos =
			{
				.x = Position.at(MU8STR("x")).Int(),
				.y = Position.at(MU8STR("y")).Int(),
				.z = Position.at(MU8STR("z")).Int(),
			};
			//获取区域大小（可能为负，结合区域偏移计算实际大小）
			const auto &Size = RgCompound.at(MU8STR("Size")).Compound();
			const BlockPos regionSize =
			{
				.x = Size.at(MU8STR("x")).Int(),
				.y = Size.at(MU8STR("y")).Int(),
				.z = Size.at(MU8STR("z")).Int(),
			};
			//计算区域实际大小
			const BlockPos posEndRel = getRelativeEndPositionFromAreaSize(regionSize).add(reginoPos);
			const BlockPos posMin = getMinCorner(reginoPos, posEndRel);
			const BlockPos posMax = getMaxCorner(reginoPos, posEndRel);
			const BlockPos size = posMax.sub(posMin).add({ 1,1,1 });

			//printf("RegionSize: [%d, %d, %d]\n", size.x, size.y, size.z);

			//获取调色板（方块种类）
			const auto &BlockStatePalette = RgCompound.at(MU8STR("BlockStatePalette")).List();
			const uint32_t bitsPerBitMapElement = Max(2U, (uint32_t)sizeof(uint32_t) * 8 - numberOfLeadingZeros(BlockStatePalette.size() - 1));//计算位图中一个元素占用的bit大小
			const uint32_t bitMaskOfElement = (1 << bitsPerBitMapElement) - 1;//获取遮罩位，用于取bitmap内部内容
			//printf("BlockStatePaletteSize: [%zu]\nbitsPerBitMapElement: [%d]\n", BlockStatePalette.size(), bitsPerBitMapElement);
			/*------------------------------------------------*/


			/*----------------方块实体获取----------------*/
			//获取方块实体列表
			const auto &TileEntities = RgCompound.at(MU8STR("TileEntities")).List();









			/*------------------------------------------------*/

			//创建方块统计vector
			std::vector<BlockStatistics> vtBlockStatistics;
			vtBlockStatistics.reserve(BlockStatePalette.size());//提前分配

			//遍历BlockStatePalette方块调色板，并从中创建等效下标的方块统计vector
			for (const auto &it : BlockStatePalette)
			{
				const auto &itCompound = it.Compound();

				BlockStatistics bsTemp{};
				bsTemp.sBlockName = itCompound.at(MU8STR("Name")).String();
				const auto find = itCompound.find(MU8STR("Properties"));//检查方块是否有额外属性
				if (find != itCompound.end())
				{
					bsTemp.cpdProperties = (*find).second.Compound();

					/*
					blockName += '[';
					const auto &Properties = (*find).second.Compound();//取出compound
					for (const auto &[ppState, ppVal] : Properties)//遍历所有额外属性并拼接到blockName后方
					{
						blockName += ppState;
						blockName += '=';
						blockName += ppVal.String();
						blockName += ',';
					}

					//如果不是空列表，替换最后一个,为]
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

			//获取Long方块状态位图数组（用于作为下标访问调色板）
			const auto &BlockStates = RgCompound.at(MU8STR("BlockStates")).Long_Array();
			const uint64_t RegionFullSize = (uint64_t)size.x * (uint64_t)size.y * (uint64_t)size.z;
			if (BlockStates.size() * 64 / bitsPerBitMapElement < RegionFullSize)
			{
				//printf("BlockStates Too Small!\n");
				vtRegionsStatistics.emplace_back(RgName,std::move(vtBlockStatistics));//全0计数
				continue;//尝试获取下一个选区
			}

			//存储格式：(y * sizeLayer) + z * sizeX + x = Long array index, sizeLayer = sizeX * sizeZ
			//遍历方块位图，设置调色板列表对应方块的计数器
			for (uint64_t startOffset = 0; startOffset < (RegionFullSize * (uint64_t)bitsPerBitMapElement); startOffset += (uint64_t)bitsPerBitMapElement)//startOffset从第二次开始递增
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

			//计数完成，插入RegionsStatistics
			vtRegionsStatistics.emplace_back(RgName,std::move(vtBlockStatistics));
		}

		return vtRegionsStatistics;
	}

	//过滤可获取的方块，不可获取的方块直接删除，半砖等特殊多物品方块转换数量
	struct ItemStack
	{
		NBT_Node::NBT_String sItemName{};//物品名
		uint64_t u64Counter = 0;//物品计数器
	};

	static ItemStack BlockStatisticsToItemStack(const BlockStatistics &stBlocks)
	{
		//先处理方块到物品转换

		//普通方块转换
		static const std::map<NBT_Node::NBT_String, uint64_t> mapFilter =//uint64_t->ID
		{
			/*0 -> minecraft:empty*/
			{MU8STR("minecraft:air"),0 },//方块air对应物品empty
			{MU8STR("minecraft:void_air"),0 },
			{MU8STR("minecraft:cave_air"),0 },

			{MU8STR("minecraft:piston_head"),0},
			{MU8STR("minecraft:moving_piston"),0},
			{MU8STR("minecraft:nether_portal"),0},
			{MU8STR("minecraft:end_portal"),0},
			{MU8STR("minecraft:end_gateway"),0},

			{MU8STR("minecraft:farmland"),1},//dirt

			{MU8STR("minecraft:lava"),2},//lava.level == 0 ? minecraft:lava_bucket : minecraft:empty
			{MU8STR("minecraft:water"),2},//water.level == 0 ? minecraft:water_bucket : minecraft:empty
		};


		//特殊方块转换
		/*
		门\床\高植物\不同种类花盆
		*/






		//方块数量转换
		/*
		半砖\雪\海龟蛋\海泡菜\蜡烛\樱花
		*/

		//特殊方块数量转换
		/*
		多面生长方块，如藤蔓，发光地衣
		*/




		return {};
	}


};