#pragma once
/*Json*/
#include <nlohmann\json.hpp>
using Json = nlohmann::json;
/*Json*/

#include "NBT_Node.hpp"

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
		bool bRet = true;
		try
		{
			data = Json::parse(pFile);
		}
		catch (const Json::parse_error &e)
		{
			// 输出异常详细信息
			printf("JSON parse error: %s\nError Pos: [%zu]\n", e.what(), e.byte);
			bRet = false;
		}

		fclose(pFile);
		return bRet;
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

	const std::string &KeyTranslate(KeyType enKeyType, const NBT_Type::String &sKeyName) const
	{
		static const std::string sKeyTypePrefix[] =
		{
			"",//未知留空
			"block.",
			"entity.",
			"item.",
		};

		if (enKeyType >= ENUM_END || enKeyType < None)
		{
			enKeyType = None;
		}

		static const std::string EmptyStr{ "" };

		static_assert(sizeof(sKeyTypePrefix) / sizeof(sKeyTypePrefix[0]) == ENUM_END);

		//拆解sKeyName并与sKeyTypePrefix[enKeyType]组合

		//把名称空间的:转换成.
		std::string sJsonKey = sKeyName.ToCharTypeUTF8();//转换到char类型并拷贝
		std::replace(sJsonKey.begin(), sJsonKey.end(), ':', '.');

	re_find:
		auto itFind = data.find(sKeyTypePrefix[enKeyType] + sJsonKey);
		if (itFind == data.end())
		{
			//是空的情况下判断是不是item，是的话需要再查一次block
			if (enKeyType == Item)//因为方块物品在block内
			{
				enKeyType = Block;//改成方块（下次不会再命中此if）
				goto re_find;//重新尝试
			}

			return EmptyStr;
		}

		//返回查找结果
		return itFind.value().get_ref<const std::string &>();
	}

};