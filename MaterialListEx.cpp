#include "MemoryLeakCheck.hpp"

#define ZLIB_CONST
#include <zlib.h>

#include <compress.hpp>
#include <config.hpp>
#include <decompress.hpp>
#include <utils.hpp>
#include <version.hpp>

#include <stdio.h>
#include <string>
#include <Windows.h>

#include "NBT_Tool.hpp"
#include "Calc_Tool.hpp"

struct BlockInfo
{
	std::string sName = {};
	size_t szBlockCount = 0;
};

bool OpenFileAndMapping(const char *pcFileName, uint8_t **pFileRet, uint64_t *pFileSizeRet)
{
	//打开输入文件并映射
	HANDLE hReadFile = CreateFileA(pcFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hReadFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	//获得文件大小
	LARGE_INTEGER liFileSize = { 0 };
	if (!GetFileSizeEx(hReadFile, &liFileSize))
	{
		CloseHandle(hReadFile);//关闭输入文件
		return false;
	}
	*pFileSizeRet = liFileSize.QuadPart;

	//判断文件为空
	if (liFileSize.QuadPart == 0)
	{
		CloseHandle(hReadFile);//关闭输入文件
		return false;
	}

	//创建文件映射对象
	HANDLE hFileMapping = CreateFileMappingW(hReadFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (!hFileMapping)
	{
		CloseHandle(hReadFile);//关闭输入文件
		return false;
	}
	CloseHandle(hReadFile);//关闭输入文件

	//映射文件到内存
	LPVOID lpReadMem = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
	if (!lpReadMem)
	{
		CloseHandle(hFileMapping);//关闭文件映射对象
		return false;
	}
	CloseHandle(hFileMapping);//关闭文件映射对象

	*pFileRet = (uint8_t *)lpReadMem;
	return true;
}

bool UnMappingAndCloseFile(uint8_t *pFileClose)
{
	if (!UnmapViewOfFile(pFileClose))
	{
		return false;
	}
	return true;
}

//#define NBT_FILE_NAME ".\\散热器水流刷怪塔（有合成器）.litematic"
#define NBT_FILE_NAME ".\\opt.nbt"

int main(void)
{
	uint8_t *pFile;
	uint64_t qwFileSize;
	if (!OpenFileAndMapping(NBT_FILE_NAME, &pFile, &qwFileSize))
	{
		return -1;
	}

	std::string nbt;

	if (gzip::is_compressed((char *)pFile, qwFileSize))//如果nbt已压缩，解压
	{
		nbt = gzip::decompress((char *)pFile, qwFileSize);
		FILE *f = fopen("opt.nbt", "wb");//覆盖输出一个解压过的文件，用于在报错发生后供分析
		if (f == NULL)
		{
			return -3;
		}
		
		if (fwrite(nbt.c_str(), nbt.size(), 1, f) != 1)
		{
			return -4;
		}
		
		fclose(f);
	}
	else//否则
	{
		//直接给数据塞string里
		nbt.resize(qwFileSize);//设置长度 c++23用resize_and_overwrite
		memcpy(nbt.data(), pFile, qwFileSize);//直接写入data
	}
	
	if (!UnMappingAndCloseFile(pFile))
	{
		return -2;
	}
	
	//以下使用nbt
	NBT_Tool nt(nbt);
	//nt.Print();
	//NBT_Node n;


	const auto &tmp = nt.GetRoot().GetData<NBT_Node::NBT_Compound>();//获取根下第一个compound，正常情况下根部下只有这一个compound
	if (tmp.size() != 1)
	{
		printf("错误的根大小");
		return -1;
	}


	const auto &root = *tmp.begin();
	//直接获取根下第一键

	printf("root:\"%s\"\n", root.first.c_str());
	
	//获取regions，也就是区域，一个投影可能有多个区域（选区）
	const auto &Regions = root.second.Compound().at("Regions").Compound();
	for (const auto &[RgName, RgVal] : Regions)//遍历选区
	{
		const auto &RgCompound = RgVal.Compound();

		//输出选区名
		printf("======%s======\n", RgName.c_str());
		//获取区域大小
		
		
		const auto &Position = RgCompound.at("Position").Compound();
		const BlockPos reginoPos =
		{
			.x = Position.at("x").Int(),
			.y = Position.at("y").Int(),
			.z = Position.at("z").Int(),
		};
		const auto &Size = RgCompound.at("Size").Compound();
		const BlockPos regionSize =
		{
			.x = Size.at("x").Int(),
			.y = Size.at("y").Int(),
			.z = Size.at("z").Int()
		};
		//计算区域大小
		const BlockPos posEndRel = getRelativeEndPositionFromAreaSize(regionSize).add(reginoPos);
		const BlockPos posMin = getMinCorner(reginoPos, posEndRel);
		const BlockPos posMax = getMaxCorner(reginoPos, posEndRel);
		const BlockPos size = posMax.sub(posMin).add({ 1,1,1 });

		printf("RegionSize:[%d %d %d]\n", size.x, size.y, size.z);

		//获取调色板（方块种类）
		const auto &BlockStatePalette = RgCompound.at("BlockStatePalette").List();
		const uint32_t bitsPerBitMapElement = Max(2U, (uint32_t)sizeof(uint32_t) * 8 - numberOfLeadingZeros(BlockStatePalette.size() - 1));//计算位图中一个元素占用的bit大小
		const uint32_t bitMaskOfElement = (1 << bitsPerBitMapElement) - 1;//获取遮罩位，用于取bitmap内部内容
		printf("BlockStatePaletteSize:[%zu]\nbitsPerBitMapElement:[%d]\n", BlockStatePalette.size(), bitsPerBitMapElement);

		//遍历BlockStatePalette，并从中创建等效下标的方块统计vector
		std::vector<BlockInfo> BlockStatistic;
		BlockStatistic.reserve(BlockStatePalette.size());//提前分配
		for (const auto &it : BlockStatePalette)
		{
			const auto &itCompound = it.Compound();

			auto blockName = itCompound.at("Name").String();
			const auto find = itCompound.find("Properties");//检查方块是否有额外属性
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
		const auto &BlockStates = RgCompound.at("BlockStates").Long_Array();
		const uint64_t RegionFullSize = (uint64_t)size.x * (uint64_t)size.y * (uint64_t)size.z;
		if (BlockStates.size() * 64 / bitsPerBitMapElement < RegionFullSize)
		{
			printf("BlockStates Too Small!\n");
			return -1;
		}

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
			printf("\"%s\": [%zu]\n", it.sName.c_str(), it.szBlockCount);
		}
	}




	printf("\nok!\n");

	return 114514;
}