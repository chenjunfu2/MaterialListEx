#pragma once
/*Json*/
#include <nlohmann\json.hpp>
using Json = nlohmann::json;
/*Json*/

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"

#include <set>
#include <string>

class Language
{
public:
	Json data{};
public:
	Language() = default;
	~Language() = default;


	bool ReadLanguageFile(const char*const pcFileName)
	{
		FILE *pFile = fopen(pcFileName, "rb");
		if (pFile == NULL)
		{
			printf("Langage file open fail\n");
			return false;
		}
		
		//从语言文件创建json对象
		try
		{
			data = Json::parse(pFile);
		}
		catch (const Json::parse_error &e)
		{
			// 输出异常详细信息
			printf("JSON parse error: %s\nError Pos: [%zu]\n", e.what(), e.byte);

			fclose(pFile);
			return false;
		}

		fclose(pFile);
		return true;
	}

	void PrintPrefixes(void) const//此函数代码由AI编写
	{
		std::set<std::string> printedPrefixes; // 用于记录已打印的前缀

		for (auto &element : data.items())
		{
			const std::string &key = element.key();
			size_t dotPos = key.find('.');

			if (dotPos == std::string::npos)
			{
				// 如果键中没有点号，整个键作为前缀
				if (printedPrefixes.find(key) == printedPrefixes.end())
				{
					printf("%s\n", key.c_str());
					printedPrefixes.insert(key);
				}
			}
			else
			{
				// 提取第一个点号前的部分作为前缀
				std::string prefix = key.substr(0, dotPos);

				// 如果此前缀未打印过，则打印并记录
				if (printedPrefixes.find(prefix) == printedPrefixes.end())
				{
					printf("%s\n", prefix.c_str());
					printedPrefixes.insert(prefix);
				}
			}
		}
	}


	enum KeyType
	{
		None = 0,//Unknown or None
		Block,
		Entity,
		Item,
		ENUM_END,
	};

	//block可能有item，item可能有block，需要预处理去掉json的前缀并且把.改成:然后直接查询而不是自己去操作拼接

	const std::string &KeyTranslate(KeyType enKeyType, const NBT_Node::NBT_String &sKeyName) const
	{
		static const NBT_Node::NBT_String sKeyTypePrefix[] =
		{
			MU8STR(""),//未知留空
			MU8STR("block."),
			MU8STR("entity."),
			MU8STR("item."),
		};

		static const std::string EmptyStr{ "" };

		static_assert(sizeof(sKeyTypePrefix) / sizeof(sKeyTypePrefix[0]) == ENUM_END);

		//拆解sKeyName并与sKeyTypePrefix[enKeyType]组合

		NBT_Node::NBT_String sJsonKey = sKeyTypePrefix[enKeyType] + sKeyName;
		size_t szPos = sKeyName.find(':');
		if (szPos != sKeyName.npos)
		{
			szPos += sKeyTypePrefix[enKeyType].size();
			sJsonKey[szPos] = '.';
		}

		auto itFind = data.find(sJsonKey);
		if (itFind == data.end())
		{
			return EmptyStr;
		}

		return itFind.value().get_ref<const std::string &>();
	}

};