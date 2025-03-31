#include "MemoryLeakCheck.hpp"

#include "Calc_Tool.hpp"
#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"
#include "NBT_Helper.hpp"
#include "NBT_Reader.hpp"
#include "NBT_Writer.hpp"

#define ZLIB_CONST
#include <zlib.h>

#include <compress.hpp>
#include <config.hpp>
#include <decompress.hpp>
#include <utils.hpp>
#include <version.hpp>

#include <stdio.h>
#include <string>

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("Only one input file is needed\n");
		return -1;
	}

	FILE *pFile = fopen(argv[1], "rb");
	if (pFile == NULL)
	{
		return -1;
	}
	//获取文件大小
	if (_fseeki64(pFile, 0, SEEK_END) != 0)
	{
		return -1;
	}
	 uint64_t qwFileSize = _ftelli64(pFile);
	 //回到文件开头
	 rewind(pFile);

	 //用于保存文件内容
	 std::string sNbtData{};
	 //直接给数据塞string里
	 sNbtData.resize(qwFileSize);//设置长度 c++23用resize_and_overwrite
	 fread(sNbtData.data(), sizeof(sNbtData[0]), qwFileSize, pFile);//直接读入data
	 //完成，关闭文件
	 fclose(pFile);
	 pFile = NULL;

	 printf("NBT file read size: [%lld]\n", qwFileSize);

	if (gzip::is_compressed(sNbtData.data(), sNbtData.size()))//如果nbt已压缩，解压，否则保持原样
	{
		sNbtData = gzip::decompress(sNbtData.data(), sNbtData.size());
		printf("NBT file decompressed size: [%lld]\n", (uint64_t)sNbtData.size());

		//路径预处理
		std::string sPath{ argv[1] };
		size_t szPos = sPath.find_last_of('.');//找到最后一个.获得后缀名前的位置
		if (szPos == std::string::npos)
		{
			szPos = sPath.size() - 1;//没有后缀就从末尾开始
		}
		//后缀名前插入自定义尾缀
		sPath.insert(szPos, "_decompress");
		printf("Output file: \"%s\" ", sPath.c_str());

		//判断文件存在性
		FILE *pTest = fopen(sPath.c_str(), "rb");
		if (pTest != NULL)
		{
			//文件已存在，不进行覆盖输出，跳过
			printf("is already exist, skipped\n");
			fclose(pTest);
			pTest = NULL;
		}
		else
		{
			//输出一个解压过的文件，用于在报错发生后供分析
			FILE *pFile = fopen(sPath.c_str(), "wb");
			if (pFile == NULL)
			{
				return -1;
			}

			if (fwrite(sNbtData.data(), sizeof(sNbtData[0]), sNbtData.size(), pFile) != sNbtData.size())
			{
				return -1;
			}

			fclose(pFile);
			pFile = NULL;

			printf("is maked successfuly\n");
		}
	}
	else
	{
		printf("NBT file is not compressed\n");
	}

	//以下使用nbt
	NBT_Node nRoot;
	if (!NBT_Reader<std::string>::ReadNBT(nRoot, sNbtData))
	{
		return -1;
	}
	else
	{
		printf("Read Ok!\n");
	}

	NBT_Helper::Print(nRoot);

	const auto &tmp = nRoot.AtCompound();//获取根下第一个compound，正常情况下根部下只有这一个compound
	if (tmp.size() != 1)
	{
		printf("Error root size");
		return -1;
	}

	//输出名称（一般是空字符串）
	const auto &root = *tmp.begin();
	printf("root:\"%s\"\n", ANSISTR(U16STR(root.first)).c_str());
	
	//获取regions，也就是区域，一个投影可能有多个区域（选区）
	const auto &Regions = root.second.Compound().at(MU8STR("Regions")).Compound();
	for (const auto &[RgName, RgVal] : Regions)//遍历选区
	{
		const auto &RgCompound = RgVal.Compound();

		//输出选区名
		printf("======%s======\n", ANSISTR(U16STR(RgName)).c_str());
		//获取区域大小
		
		
		const auto &Position = RgCompound.at(MU8STR("Position")).Compound();
		const BlockPos reginoPos =
		{
			.x = Position.at(MU8STR("x")).Int(),
			.y = Position.at(MU8STR("y")).Int(),
			.z = Position.at(MU8STR("z")).Int(),
		};
		const auto &Size = RgCompound.at(MU8STR("Size")).Compound();
		const BlockPos regionSize =
		{
			.x = Size.at(MU8STR("x")).Int(),
			.y = Size.at(MU8STR("y")).Int(),
			.z = Size.at(MU8STR("z")).Int(),
		};
		//计算区域大小
		const BlockPos posEndRel = getRelativeEndPositionFromAreaSize(regionSize).add(reginoPos);
		const BlockPos posMin = getMinCorner(reginoPos, posEndRel);
		const BlockPos posMax = getMaxCorner(reginoPos, posEndRel);
		const BlockPos size = posMax.sub(posMin).add({ 1,1,1 });

		printf("RegionSize: [%d, %d, %d]\n", size.x, size.y, size.z);

		//获取调色板（方块种类）
		const auto &BlockStatePalette = RgCompound.at(MU8STR("BlockStatePalette")).List();
		const uint32_t bitsPerBitMapElement = Max(2U, (uint32_t)sizeof(uint32_t) * 8 - numberOfLeadingZeros(BlockStatePalette.size() - 1));//计算位图中一个元素占用的bit大小
		const uint32_t bitMaskOfElement = (1 << bitsPerBitMapElement) - 1;//获取遮罩位，用于取bitmap内部内容
		printf("BlockStatePaletteSize: [%zu]\nbitsPerBitMapElement: [%d]\n", BlockStatePalette.size(), bitsPerBitMapElement);

		//遍历BlockStatePalette，并从中创建等效下标的方块统计vector
		struct BlockInfo
		{
			std::string sName = {};
			size_t szBlockCount = 0;
		};
		std::vector<BlockInfo> BlockStatistic;
		BlockStatistic.reserve(BlockStatePalette.size());//提前分配
		for (const auto &it : BlockStatePalette)
		{
			const auto &itCompound = it.Compound();

			auto blockName = itCompound.at(MU8STR("Name")).String();
			const auto find = itCompound.find(MU8STR("Properties"));//检查方块是否有额外属性
			if (find != itCompound.end())
			{
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
			}

			BlockStatistic.emplace_back(BlockInfo{ std::move(blockName),0 });
		}

		//获取Long方块状态位图数组（用于作为下标访问调色板）
		const auto &BlockStates = RgCompound.at(MU8STR("BlockStates")).Long_Array();
		const uint64_t RegionFullSize = (uint64_t)size.x * (uint64_t)size.y * (uint64_t)size.z;
		if (BlockStates.size() * 64 / bitsPerBitMapElement < RegionFullSize)
		{
			printf("BlockStates Too Small!\n");
			return -1;
		}

		//存储格式：(y * sizeLayer) + z * sizeX + x = Long array index, sizeLayer = sizeX * sizeZ

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

			++BlockStatistic[paletteIndex].szBlockCount;
		}

		for (const auto &it : BlockStatistic)
		{
			printf("\"%s\": [%zu]\n", ANSISTR(U16STR(it.sName)).c_str(), it.szBlockCount);
		}
	}

	printf("\nOk!\n");

	return 0;
}

//创建方块状态到物品映射map