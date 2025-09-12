#pragma once

#include "NBT_Node.hpp"
#include "NBT_Node_View.hpp"

#include <concepts>

#include <bit>
#include <xxhash.h>

class NBT_Helper
{
public:
	NBT_Helper() = delete;
	~NBT_Helper() = delete;
public:
	static void Print(const NBT_Node_View<true> nRoot, bool bPadding = true, bool bNewLine = true)
	{
		size_t szLevelStart = bPadding ? 0 : (size_t)-1;//������ӡ

		PrintSwitch(nRoot, 0);
		if (bNewLine)
		{
			putchar('\n');
		}
	}

	static std::string Serialize(const NBT_Node_View<true> nRoot)
	{
		std::string sRet{};
		SerializeSwitch(nRoot, sRet);
		return sRet;
	}

	static void DefaultFunc(XXH64_state_t *)
	{
		return;
	}

	using DefaultFuncType = std::decay_t<decltype(DefaultFunc)>;

	template<typename TS = DefaultFuncType, typename TE = DefaultFuncType>//�����������ֱ���ǰ����ã��������ڲ����ϣ����
	static XXH64_hash_t Hash(const NBT_Node_View<true> nRoot, XXH64_hash_t u64Seed, TS funS = DefaultFunc, TE funE = DefaultFunc)
	{
		XXH64_state_t * pHashState = XXH64_createState();
		XXH64_reset(pHashState, u64Seed);

		funS(pHashState);
		HashSwitch<true>(nRoot, pHashState);
		funE(pHashState);

		XXH64_hash_t hashNBT = XXH64_digest(pHashState);
		XXH64_freeState(pHashState);
		return hashNBT;

		using DefaultFuncArg = std::decay_t<decltype(pHashState)>;
		static_assert(std::is_invocable_v<TS, DefaultFuncArg>, "TS is not a callable object or parameter type mismatch.");
		static_assert(std::is_invocable_v<TE, DefaultFuncArg>, "TE is not a callable object or parameter type mismatch.");
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
			printf("%s", LevelPadding);
		}

		if (bSubLevel)
		{
			printf("%s", LevelPadding);
		}
	}

	//�״ε���Ĭ��Ϊtrue�����ε��ÿ�ʼ�ڲ�������Ϊfalse
	template<bool bRoot = true>//�״�ʹ��NBT_Node_View���������ֱ��ʹ��NBT_Node������������ʼ������
	static void PrintSwitch(std::conditional_t<bRoot, const NBT_Node_View<true> &, const NBT_Node &>nRoot, size_t szLevel)
	{
		auto tag = nRoot.GetTag();
		switch (tag)
		{
		case NBT_TAG::End:
			{
				printf("[End]");
			}
			break;
		case NBT_TAG::Byte:
			{
				printf("%dB", nRoot.GetData<NBT_Type::Byte>());
			}
			break;
		case NBT_TAG::Short:
			{
				printf("%dS", nRoot.GetData<NBT_Type::Short>());
			}
			break;
		case NBT_TAG::Int:
			{
				printf("%dI", nRoot.GetData<NBT_Type::Int>());
			}
			break;
		case NBT_TAG::Long:
			{
				printf("%lldL", nRoot.GetData<NBT_Type::Long>());
			}
			break;
		case NBT_TAG::Float:
			{
				printf("%gF", nRoot.GetData<NBT_Type::Float>());
			}
			break;
		case NBT_TAG::Double:
			{
				printf("%gD", nRoot.GetData<NBT_Type::Double>());
			}
			break;
		case NBT_TAG::Byte_Array:
			{
				const auto &arr = nRoot.GetData<NBT_Type::ByteArray>();
				printf("[B;");
				for (const auto &it : arr)
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
		case NBT_TAG::Int_Array:
			{
				const auto &arr = nRoot.GetData<NBT_Type::IntArray>();
				printf("[I;");
				for (const auto &it : arr)
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
		case NBT_TAG::Long_Array:
			{
				const auto &arr = nRoot.GetData<NBT_Type::LongArray>();
				printf("[L;");
				for (const auto &it : arr)
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
		case NBT_TAG::String:
			{
				printf("\"%s\"", U16ANSI(U16STR(nRoot.GetData<NBT_Type::String>())).c_str());
			}
			break;
		case NBT_TAG::List://��Ҫ��ӡ�����ĵط�
			{
				const auto &list = nRoot.GetData<NBT_Type::List>();
				PrintPadding(szLevel, false, !bRoot);//���Ǹ������ӡ��ͷ����
				printf("[");
				for (const auto &it : list)
				{
					PrintPadding(szLevel, true, it.GetTag() != NBT_TAG::Compound && it.GetTag() != NBT_TAG::List);
					PrintSwitch<false>(it, szLevel + 1);
					printf(",");
				}

				if (list.Size() != 0)
				{
					printf("\b \b");//������һ������
					PrintPadding(szLevel, false, true);//���б����軻���Լ�����
				}
				printf("]");
			}
			break;
		case NBT_TAG::Compound://��Ҫ��ӡ�����ĵط�
			{
				const auto &cpd = nRoot.GetData<NBT_Type::Compound>();
				PrintPadding(szLevel, false, !bRoot);//���Ǹ������ӡ��ͷ����
				printf("{");

				for (const auto &it : cpd)
				{
					PrintPadding(szLevel, true, true);
					printf("\"%s\":", U16ANSI(U16STR(it.first)).c_str());
					PrintSwitch<false>(it.second, szLevel + 1);
					printf(",");
				}

				if (cpd.Size() != 0)
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

	//�״ε���Ĭ��Ϊtrue�����ε��ÿ�ʼ�ڲ�������Ϊfalse
	template<bool bRoot = true>//�״�ʹ��NBT_Node_View���������ֱ��ʹ��NBT_Node������������ʼ������
	static void SerializeSwitch(std::conditional_t<bRoot, const NBT_Node_View<true> &, const NBT_Node &>nRoot, std::string &sRet)
	{
		auto tag = nRoot.GetTag();
		switch (tag)
		{
		case NBT_TAG::End:
			{
				sRet += "[End]";
			}
			break;
		case NBT_TAG::Byte:
			{
				ToHexString(nRoot.GetData<NBT_Type::Byte>(), sRet);
				sRet += 'B';
			}
			break;
		case NBT_TAG::Short:
			{
				ToHexString(nRoot.GetData<NBT_Type::Short>(), sRet);
				sRet += 'S';
			}
			break;
		case NBT_TAG::Int:
			{
				ToHexString(nRoot.GetData<NBT_Type::Int>(), sRet);
				sRet += 'I';
			}
			break;
		case NBT_TAG::Long:
			{
				ToHexString(nRoot.GetData<NBT_Type::Long>(), sRet);
				sRet += 'L';
			}
			break;
		case NBT_TAG::Float:
			{
				ToHexString(nRoot.GetData<NBT_Type::Float>(), sRet);
				sRet += 'F';
			}
			break;
		case NBT_TAG::Double:
			{
				ToHexString(nRoot.GetData<NBT_Type::Double>(), sRet);
				sRet += 'D';
			}
			break;
		case NBT_TAG::Byte_Array:
			{
				const auto &arr = nRoot.GetData<NBT_Type::ByteArray>();
				sRet += "[B;";
				for (const auto &it : arr)
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
		case NBT_TAG::Int_Array:
			{
				const auto &arr = nRoot.GetData<NBT_Type::IntArray>();
				sRet += "[I;";
				for (const auto &it : arr)
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
		case NBT_TAG::Long_Array:
			{
				const auto &arr = nRoot.GetData<NBT_Type::LongArray>();
				sRet += "[L;";
				for (const auto &it : arr)
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
		case NBT_TAG::String:
			{
				sRet += '\"';
				sRet += nRoot.GetData<NBT_Type::String>();
				sRet += '\"';
			}
			break;
		case NBT_TAG::List:
			{
				const auto &list = nRoot.GetData<NBT_Type::List>();
				sRet += '[';
				for (const auto &it : list)
				{
					SerializeSwitch<false>(it, sRet);
					sRet += ',';
				}
	
				if (list.Size() != 0)
				{
					sRet.pop_back();//ɾ�����һ������
				}
				sRet += ']';
			}
			break;
		case NBT_TAG::Compound://��Ҫ��ӡ�����ĵط�
			{
				const auto &cpd = nRoot.GetData<NBT_Type::Compound>();
				sRet += '{';
	
				for (const auto &it : cpd)
				{
					sRet += '\"';
					sRet += it.first;
					sRet += "\":";
					SerializeSwitch<false>(it.second, sRet);
					sRet += ',';
				}
	
				if (cpd.Size() != 0)
				{
					sRet.pop_back();//ɾ�����һ������
				}
				sRet += '}';
			}
			break;
		default:
			{
				sRet += "[Unknown NBT Tag Type [";
				ToHexString((NBT_TAG_RAW_TYPE)tag, sRet);
				sRet += "]]";
			}
			break;
		}
	}

private:
	template<bool bRoot = false>//�״�ʹ��NBT_Node_View���������ֱ��ʹ��NBT_Node������������ʼ������
	static void HashSwitch(std::conditional_t<bRoot, const NBT_Node_View<true> &, const NBT_Node &>nRoot, XXH64_state_t *pHashState)
	{
		auto tag = nRoot.GetTag();

		//��tag������Ϊ����
		{
			const auto &tmp = tag;
			XXH64_update(pHashState, &tmp, sizeof(tmp));
		}

		//�ٶ���ʵ��������Ϊ����
		switch (tag)
		{
		case NBT_TAG::End:
			{
				//end�����޸��أ�����ʲôҲ����
			}
			break;
		case NBT_TAG::Byte:
			{
				const auto &tmp = nRoot.GetData<NBT_Type::Byte>();
				XXH64_update(pHashState, &tmp, sizeof(tmp));
			}
			break;
		case NBT_TAG::Short:
			{
				const auto &tmp = nRoot.GetData<NBT_Type::Short>();
				XXH64_update(pHashState, &tmp, sizeof(tmp));
			}
			break;
		case NBT_TAG::Int:
			{
				const auto &tmp = nRoot.GetData<NBT_Type::Int>();
				XXH64_update(pHashState, &tmp, sizeof(tmp));
			}
			break;
		case NBT_TAG::Long:
			{
				const auto &tmp = nRoot.GetData<NBT_Type::Long>();
				XXH64_update(pHashState, &tmp, sizeof(tmp));
			}
			break;
		case NBT_TAG::Float:
			{
				const auto &tmp = nRoot.GetData<NBT_Type::Float>();
				XXH64_update(pHashState, &tmp, sizeof(tmp));
			}
			break;
		case NBT_TAG::Double:
			{
				const auto &tmp = nRoot.GetData<NBT_Type::Double>();
				XXH64_update(pHashState, &tmp, sizeof(tmp));
			}
			break;
		case NBT_TAG::Byte_Array:
			{
				const auto &arr = nRoot.GetData<NBT_Type::ByteArray>();
				for (const auto &it : arr)
				{
					const auto &tmp = it;
					XXH64_update(pHashState, &tmp, sizeof(tmp));
				}
			}
			break;
		case NBT_TAG::Int_Array:
			{
				const auto &arr = nRoot.GetData<NBT_Type::IntArray>();
				for (const auto &it : arr)
				{
					const auto &tmp = it;
					XXH64_update(pHashState, &tmp, sizeof(tmp));
				}
			}
			break;
		case NBT_TAG::Long_Array:
			{
				const auto &arr = nRoot.GetData<NBT_Type::LongArray>();
				for (const auto &it : arr)
				{
					const auto &tmp = it;
					XXH64_update(pHashState, &tmp, sizeof(tmp));
				}
			}
			break;
		case NBT_TAG::String:
			{
				const auto &tmp = nRoot.GetData<NBT_Type::String>();
				XXH64_update(pHashState, tmp.data(), tmp.size());
			}
			break;
		case NBT_TAG::List:
			{
				const auto &list = nRoot.GetData<NBT_Type::List>();
				for (const auto &it : list)
				{
					HashSwitch(it, pHashState);
				}
			}
			break;
		case NBT_TAG::Compound://��Ҫ��ӡ�����ĵط�
			{
				const auto &cpd = nRoot.GetData<NBT_Type::Compound>();
				for (const auto &it : cpd)
				{
					const auto &tmp = it.first;
					XXH64_update(pHashState, tmp.data(), tmp.size());
					HashSwitch(it.second, pHashState);
				}
			}
			break;
		default:
			{}
			break;
		}
	}


};