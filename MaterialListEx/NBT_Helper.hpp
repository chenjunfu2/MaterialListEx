#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"

class NBT_Helper
{
public:
	NBT_Helper() = delete;
	~NBT_Helper() = delete;
public:
	static void Print(const NBT_Node &nRoot, bool bPadding = true, bool bNewLine = true)
	{
		size_t szLevelStart = bPadding ? 0 : (size_t)-1;//跳过打印

		PrintSwitch(nRoot, 0);
		if (bNewLine)
		{
			printf("\n");
		}
	}

private:
	constexpr const static inline char *const LevelPadding = "    ";

	static void PrintPadding(size_t szLevel, bool bSubLevel, bool bNewLine)//bSubLevel会让缩进多一层
	{
		if (szLevel == (size_t)-1)//跳过打印
		{
			return;
		}

		if (bNewLine)
		{
			putchar('\n');
		}
		
		for (size_t i = 0; i < szLevel; ++i)
		{
			printf(LevelPadding);
		}

		if (bSubLevel)
		{
			printf(LevelPadding);
		}
	}

	static void PrintSwitch(const NBT_Node &nRoot, size_t szLevel)
	{
		auto tag = nRoot.GetTag();
		switch (tag)
		{
		case NBT_Node::TAG_End:
			{
				printf("[Compound End]");
			}
			break;
		case NBT_Node::TAG_Byte:
			{
				printf("%db", nRoot.GetData<NBT_Node::NBT_Byte>());
			}
			break;
		case NBT_Node::TAG_Short:
			{
				printf("%ds", nRoot.GetData<NBT_Node::NBT_Short>());
			}
			break;
		case NBT_Node::TAG_Int:
			{
				printf("%d", nRoot.GetData<NBT_Node::NBT_Int>());
			}
			break;
		case NBT_Node::TAG_Long:
			{
				printf("%lldl", nRoot.GetData<NBT_Node::NBT_Long>());
			}
			break;
		case NBT_Node::TAG_Float:
			{
				printf("%ff", nRoot.GetData<NBT_Node::NBT_Float>());
			}
			break;
		case NBT_Node::TAG_Double:
			{
				printf("%lff", nRoot.GetData<NBT_Node::NBT_Double>());
			}
			break;
		case NBT_Node::TAG_Byte_Array:
			{
				auto &arr = nRoot.GetData<NBT_Node::NBT_ByteArray>();
				printf("[B;");
				for (auto &it : arr)
				{
					printf("%d,", it);
				}
				if (arr.size() != 0)
				{
					printf("\b");
				}

				printf("]");
			}
			break;
		case NBT_Node::TAG_String:
			{
				printf("\"%s\"", ANSISTR(U16STR(nRoot.GetData<NBT_Node::NBT_String>())).c_str());
			}
			break;
		case NBT_Node::TAG_List://需要打印缩进的地方
			{
				auto &list = nRoot.GetData<NBT_Node::NBT_List>();
				PrintPadding(szLevel, false, true);
				printf("[");
				for (auto &it : list)
				{
					PrintPadding(szLevel, true, it.GetTag() != NBT_Node::TAG_Compound && it.GetTag() != NBT_Node::TAG_List);
					PrintSwitch(it, szLevel + 1);
					printf(",");
				}

				if (list.size() != 0)
				{
					printf("\b \b");//清除最后一个逗号
					PrintPadding(szLevel, false, true);//空列表无需换行以及对齐
				}
				printf("]");
			}
			break;
		case NBT_Node::TAG_Compound://需要打印缩进的地方
			{
				auto &cpd = nRoot.GetData<NBT_Node::NBT_Compound>();
				PrintPadding(szLevel, false, true);
				printf("{");

				for (auto &it : cpd)
				{
					PrintPadding(szLevel, true, true);
					printf("\"%s\":", ANSISTR(U16STR(it.first)).c_str());
					PrintSwitch(it.second, szLevel + 1);
					printf(",");
				}

				if (cpd.size() != 0)
				{
					printf("\b \b");//清除最后一个逗号
					PrintPadding(szLevel, false, true);//空集合无需换行以及对齐
				}
				printf("}");
			}
			break;
		case NBT_Node::TAG_Int_Array:
			{
				auto &arr = nRoot.GetData<NBT_Node::NBT_IntArray>();
				printf("[I;");
				for (auto &it : arr)
				{
					printf("%d,", it);
				}

				if (arr.size() != 0)
				{
					printf("\b");
				}
				printf("]");
			}
			break;
		case NBT_Node::TAG_Long_Array:
			{
				auto &arr = nRoot.GetData<NBT_Node::NBT_LongArray>();
				printf("[L;");
				for (auto &it : arr)
				{
					printf("%lld,", it);
				}

				if (arr.size() != 0)
				{
					printf("\b");
				}
				printf("]");
			}
			break;
		default:
			{
				printf("[Unknown NBT Tag Type [%02X(%d)]]", tag, tag);
			}
			break;
		}
	}

};