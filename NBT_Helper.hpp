#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"

class NBT_Helper
{
public:
	static void Print(const NBT_Node &nRoot, bool bNewLine = true)
	{
		PrintSwitch(nRoot, 0);
		if (bNewLine)
		{
			printf("\n");
		}
	}

private:
	static void PrintSwitch(const NBT_Node &nRoot, int iLevel)
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
				auto &arr = nRoot.GetData<NBT_Node::NBT_Byte_Array>();
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
		case NBT_Node::TAG_List:
			{
				auto &list = nRoot.GetData<NBT_Node::NBT_List>();
				printf("[");
				for (auto &it : list)
				{
					PrintSwitch(it, ++iLevel);
					printf(",");
				}

				if (list.size() != 0)
				{
					printf("\b");
				}
				printf("]");
			}
			break;
		case NBT_Node::TAG_Compound:
			{
				auto &cpd = nRoot.GetData<NBT_Node::NBT_Compound>();
				printf("{");

				for (auto &it : cpd)
				{
					printf("\"%s\":", ANSISTR(U16STR(it.first)).c_str());
					PrintSwitch(it.second, ++iLevel);
					printf(",");
				}

				if (cpd.size() != 0)
				{
					printf("\b");
				}
				printf("}");
			}
			break;
		case NBT_Node::TAG_Int_Array:
			{
				auto &arr = nRoot.GetData<NBT_Node::NBT_Int_Array>();
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
				auto &arr = nRoot.GetData<NBT_Node::NBT_Long_Array>();
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
				printf("[Unknow NBT Tag Type [%02X(%d)]]", tag, tag);
			}
			break;
		}
	}





};