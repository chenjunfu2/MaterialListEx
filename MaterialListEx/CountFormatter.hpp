#include <string>
#include <stdint.h>
#include <stddef.h>//size_t

class CountFormatter
{
public:
	CountFormatter(void) = delete;
	~CountFormatter(void) = delete;

public:
	struct Level
	{
		union
		{
			struct
			{
				uint64_t u64LargeChestShulkerBox;
				uint64_t u64ChestShulkerBox;
				uint64_t u64ShulkerBox;
				uint64_t u64SetItem;
				uint64_t u64Item;
			};
			uint64_t u64Data[5];
		};
		static inline constexpr const size_t szDatacount = sizeof(u64Data) / sizeof(u64Data[0]);
	};

private:
	static inline constexpr const char *const cLevel[Level::szDatacount] =
	{
		"大箱盒","箱盒","盒","组","个",
	};

	enum CellCount
	{
		ChestCellCount = 27,
		ShulkerBoxCellCount = ChestCellCount,
		LargeChestCellCount = ChestCellCount * 2,
	};

	enum ItemCount
	{
		SetItemCount = 64,
		ShulkerBoxItemCount = ShulkerBoxCellCount * SetItemCount,
		ChestShulkerBoxItemCount = ChestCellCount * ShulkerBoxCellCount * SetItemCount,
		LargeChestShulkerBoxItemCount = LargeChestCellCount * ShulkerBoxCellCount * SetItemCount,
	};

public:
	static Level CalculateLevels(uint64_t u64Count)
	{
		Level level = { 0 };

		//挨个计算
		if (u64Count >= LargeChestShulkerBoxItemCount)
		{
			level.u64LargeChestShulkerBox = (u64Count / LargeChestShulkerBoxItemCount);
		}
		if (u64Count >= ChestShulkerBoxItemCount)
		{
			level.u64ChestShulkerBox = (u64Count / ChestShulkerBoxItemCount) % (LargeChestCellCount / ChestCellCount);
		}
		if (u64Count >= ShulkerBoxItemCount)
		{
			level.u64ShulkerBox = (u64Count / ShulkerBoxItemCount) % ShulkerBoxCellCount;
		}
		if (u64Count >= SetItemCount)
		{
			level.u64SetItem = (u64Count / SetItemCount) % ShulkerBoxCellCount;
		}
		//if (u64Count > 0)//无需判断
		{
			level.u64Item = u64Count % SetItemCount;
		}

		return level;
	}


	static std::string Level2String(const Level &level)
	{
		std::string strRet;
		strRet.reserve(36);//预分配

		//挨个输出非0项
		bool bOut = false;
		for (int i = 0; i < Level::szDatacount; ++i)
		{
			if (level.u64Data[i] == 0)
			{
				continue;
			}

			if (bOut)
			{
				strRet += " + ";
			}
			strRet += std::to_string(level.u64Data[i]);//转换数值到string
			strRet += cLevel[i];//加上单位
			bOut = true;
		}

		if (!bOut)//没一个非0（没有输出过）
		{
			strRet += '0';//输出0个
			strRet += cLevel[Level::szDatacount - 1];//单位“个”
		}

		return strRet;
	}
};

