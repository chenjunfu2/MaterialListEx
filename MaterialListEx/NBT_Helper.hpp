#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"
#include <bit>

class NBT_Helper
{
public:
	NBT_Helper() = delete;
	~NBT_Helper() = delete;
public:
	static void Print(const NBT_Node_View<true> nRoot, bool bPadding = true, bool bNewLine = true)
	{
		size_t szLevelStart = bPadding ? 0 : (size_t)-1;//������ӡ

		PrintSwitch<true>(nRoot, 0);
		if (bNewLine)
		{
			printf("\n");
		}
	}

	static std::string Serialize(const NBT_Node_View<true> nRoot)
	{
		std::string sRet{};
		SerializeSwitch<true>(nRoot, sRet);
		return sRet;
	}
private:
	constexpr const static inline char *const LevelPadding = "    ";

	static void PrintPadding(size_t szLevel, bool bSubLevel, bool bNewLine)//bSubLevel����������һ��
	{
		if (szLevel == (size_t)-1)//������ӡ
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

	template<bool bRoot = false>//�״�ʹ��NBT_Node_View���������ֱ��ʹ��NBT_Node������������ʼ������
	static void PrintSwitch(std::conditional_t<bRoot, const NBT_Node_View<true> &, const NBT_Node &>nRoot, size_t szLevel)
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
				printf("%dB", nRoot.GetData<NBT_Node::NBT_Byte>());
			}
			break;
		case NBT_Node::TAG_Short:
			{
				printf("%dS", nRoot.GetData<NBT_Node::NBT_Short>());
			}
			break;
		case NBT_Node::TAG_Int:
			{
				printf("%dI", nRoot.GetData<NBT_Node::NBT_Int>());
			}
			break;
		case NBT_Node::TAG_Long:
			{
				printf("%lldL", nRoot.GetData<NBT_Node::NBT_Long>());
			}
			break;
		case NBT_Node::TAG_Float:
			{
				printf("%gF", nRoot.GetData<NBT_Node::NBT_Float>());
			}
			break;
		case NBT_Node::TAG_Double:
			{
				printf("%gD", nRoot.GetData<NBT_Node::NBT_Double>());
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
		case NBT_Node::TAG_String:
			{
				printf("\"%s\"", ANSISTR(U16STR(nRoot.GetData<NBT_Node::NBT_String>())).c_str());
			}
			break;
		case NBT_Node::TAG_List://��Ҫ��ӡ�����ĵط�
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
					printf("\b \b");//������һ������
					PrintPadding(szLevel, false, true);//���б����軻���Լ�����
				}
				printf("]");
			}
			break;
		case NBT_Node::TAG_Compound://��Ҫ��ӡ�����ĵط�
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
					printf("\b \b");//������һ������
					PrintPadding(szLevel, false, true);//�ռ������軻���Լ�����
				}
				printf("}");
			}
			break;
		default:
			{
				printf("[Unknown NBT Tag Type [%02X(%d)]]", tag, tag);
			}
			break;
		}
	}

private:
	template<typename T>
	static void ToHexString(const T &value, std::string &result)
	{
		static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");
		static constexpr char hex_chars[] = "0123456789ABCDEF";

		//���̶��ֽ�����
		const unsigned char *bytes = (const unsigned char *)&value;
		if constexpr (std::endian::native == std::endian::little)
		{
			for (size_t i = sizeof(T); i-- > 0; )
			{
				result += hex_chars[(bytes[i] >> 4) & 0x0F];//��4
				result += hex_chars[(bytes[i] >> 0) & 0x0F];//��4
			}
		}
		else
		{
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				result += hex_chars[(bytes[i] >> 4) & 0x0F];//��4
				result += hex_chars[(bytes[i] >> 0) & 0x0F];//��4
			}
		}
	}

template<bool bRoot = false>//�״�ʹ��NBT_Node_View���������ֱ��ʹ��NBT_Node������������ʼ������
static void SerializeSwitch(std::conditional_t<bRoot, const NBT_Node_View<true> &, const NBT_Node &>nRoot, std::string &sRet)
{
	auto tag = nRoot.GetTag();
	switch (tag)
	{
	case NBT_Node::TAG_End:
		{
			sRet += "[Compound End]";
		}
		break;
	case NBT_Node::TAG_Byte:
		{
			ToHexString(nRoot.GetData<NBT_Node::NBT_Byte>(), sRet);
			sRet += 'B';
		}
		break;
	case NBT_Node::TAG_Short:
		{
			ToHexString(nRoot.GetData<NBT_Node::NBT_Short>(), sRet);
			sRet += 'S';
		}
		break;
	case NBT_Node::TAG_Int:
		{
			ToHexString(nRoot.GetData<NBT_Node::NBT_Int>(), sRet);
			sRet += 'I';
		}
		break;
	case NBT_Node::TAG_Long:
		{
			ToHexString(nRoot.GetData<NBT_Node::NBT_Long>(), sRet);
			sRet += 'L';
		}
		break;
	case NBT_Node::TAG_Float:
		{
			ToHexString(nRoot.GetData<NBT_Node::NBT_Float>(), sRet);
			sRet += 'F';
		}
		break;
	case NBT_Node::TAG_Double:
		{
			ToHexString(nRoot.GetData<NBT_Node::NBT_Double>(), sRet);
			sRet += 'D';
		}
		break;
	case NBT_Node::TAG_Byte_Array:
		{
			auto &arr = nRoot.GetData<NBT_Node::NBT_ByteArray>();
			sRet += "[B;";
			for (auto &it : arr)
			{
				ToHexString(it, sRet);
				sRet += ',';
			}
			if (arr.size() != 0)
			{
				sRet.pop_back();//ɾ�����һ������
			}

			sRet += ']';
		}
		break;
	case NBT_Node::TAG_Int_Array:
		{
			auto &arr = nRoot.GetData<NBT_Node::NBT_IntArray>();
			sRet += "[I;";
			for (auto &it : arr)
			{
				ToHexString(it, sRet);
				sRet += ',';
			}
			if (arr.size() != 0)
			{
				sRet.pop_back();//ɾ�����һ������
			}

			sRet += ']';
		}
		break;
	case NBT_Node::TAG_Long_Array:
		{
			auto &arr = nRoot.GetData<NBT_Node::NBT_LongArray>();
			sRet += "[L;";
			for (auto &it : arr)
			{
				ToHexString(it, sRet);
				sRet += ',';
			}
			if (arr.size() != 0)
			{
				sRet.pop_back();//ɾ�����һ������
			}

			sRet += ']';
		}
		break;
	case NBT_Node::TAG_String:
		{
			sRet += '\"';
			sRet += nRoot.GetData<NBT_Node::NBT_String>();
			sRet += '\"';
		}
		break;
	case NBT_Node::TAG_List:
		{
			auto &list = nRoot.GetData<NBT_Node::NBT_List>();
			sRet += '[';
			for (auto &it : list)
			{
				SerializeSwitch(it, sRet);
				sRet += ',';
			}

			if (list.size() != 0)
			{
				sRet.pop_back();//ɾ�����һ������
			}
			sRet += ']';
		}
		break;
	case NBT_Node::TAG_Compound://��Ҫ��ӡ�����ĵط�
		{
			auto &cpd = nRoot.GetData<NBT_Node::NBT_Compound>();
			sRet += '{';

			for (auto &it : cpd)
			{
				sRet += '\"';
				sRet += it.first;
				sRet += "\":";
				SerializeSwitch(it.second, sRet);
				sRet += ',';
			}

			if (cpd.size() != 0)
			{
				sRet.pop_back();//ɾ�����һ������
			}
			sRet += '}';
		}
		break;
	default:
		{
			sRet += "[Unknown NBT Tag Type [";
			ToHexString((NBT_Node::NBT_TAG_RAW_TYPE)tag, sRet);
			sRet += "]]";
		}
		break;
	}
}


};