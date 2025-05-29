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
		
		//�������ļ�����json����
		try
		{
			data = Json::parse(pFile);
		}
		catch (const Json::parse_error &e)
		{
			// ����쳣��ϸ��Ϣ
			printf("JSON parse error: %s\nError Pos: [%zu]\n", e.what(), e.byte);

			fclose(pFile);
			return false;
		}

		fclose(pFile);
		return true;
	}

	void PrintPrefixes(void) const//�˺���������AI��д
	{
		std::set<std::string> printedPrefixes; // ���ڼ�¼�Ѵ�ӡ��ǰ׺

		for (auto &element : data.items())
		{
			const std::string &key = element.key();
			size_t dotPos = key.find('.');

			if (dotPos == std::string::npos)
			{
				// �������û�е�ţ���������Ϊǰ׺
				if (printedPrefixes.find(key) == printedPrefixes.end())
				{
					printf("%s\n", key.c_str());
					printedPrefixes.insert(key);
				}
			}
			else
			{
				// ��ȡ��һ�����ǰ�Ĳ�����Ϊǰ׺
				std::string prefix = key.substr(0, dotPos);

				// �����ǰ׺δ��ӡ�������ӡ����¼
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

	//block������item��item������block����ҪԤ����ȥ��json��ǰ׺���Ұ�.�ĳ�:Ȼ��ֱ�Ӳ�ѯ�������Լ�ȥ����ƴ��

	const std::string &KeyTranslate(KeyType enKeyType, const NBT_Node::NBT_String &sKeyName) const
	{
		static const NBT_Node::NBT_String sKeyTypePrefix[] =
		{
			MU8STR(""),//δ֪����
			MU8STR("block."),
			MU8STR("entity."),
			MU8STR("item."),
		};

		static const std::string EmptyStr{ "" };

		static_assert(sizeof(sKeyTypePrefix) / sizeof(sKeyTypePrefix[0]) == ENUM_END);

		//���sKeyName����sKeyTypePrefix[enKeyType]���

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