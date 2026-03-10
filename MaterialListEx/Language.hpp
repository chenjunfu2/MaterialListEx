#pragma once


#include <NBT_Node.hpp>

#include <set>
#include <string>

#include "FileSystemHelper.hpp"

class Language
{
public:
	Json data{};
public:
	Language() = default;
	~Language() = default;


	bool ReadLanguageFile(const std::string &strFileName)
	{
		return ReadJsonFile(strFileName, data);
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

	const std::string &KeyTranslate(KeyType enKeyType, std::string sKeyName) const//拷贝名称，因为需要修改
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
		std::replace(sKeyName.begin(), sKeyName.end(), ':', '.');

	re_find:
		auto itFind = data.find(sKeyTypePrefix[enKeyType] + sKeyName);
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