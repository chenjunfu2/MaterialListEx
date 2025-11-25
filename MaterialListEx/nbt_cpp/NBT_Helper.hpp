#pragma once

#include <bit>
#include <concepts>
#include <iterator>
#include <algorithm>

#include "NBT_Print.hpp"//打印输出
#include "NBT_Node.hpp"
#include "NBT_Node_View.hpp"

#include "vcpkg_config.h"//包含vcpkg生成的配置以确认库安装情况

#ifdef CJF2_NBT_CPP_USE_XXHASH
#include "NBT_Hash.hpp"
#endif

/// @file
/// @brief NBT对象辅助工具集

/// @brief 用于格式化打印、序列化、计算哈希等功能
/// @note 计算哈希需要安装xxhash库
class NBT_Helper
{
	/// @brief 禁止构造
	NBT_Helper(void) = delete;
	/// @brief 禁止析构
	~NBT_Helper(void) = delete;

public:
	/// @brief 格式化对齐打印NBT对象
	/// @tparam bSortCompound 是否对Compound进行排序
	/// @tparam PrintFunc 用于输出的仿函数类型，具体格式请参考NBT_Print说明
	/// @param nRoot 任意NBT_Type中的类型，仅初始化为视图
	/// @param funcPrint 用于输出的仿函数
	/// @param bPadding 是否打印缩进补白
	/// @param bNewLine 是否在所有打印完成后的末尾换行
	template<bool bSortCompound = true, typename PrintFunc = NBT_Print>
	static void Print(const NBT_Node_View<true> nRoot, PrintFunc funcPrint = NBT_Print{ stdout }, bool bPadding = true, bool bNewLine = true)
	{
		size_t szLevelStart = bPadding ? 0 : (size_t)-1;//跳过打印

		PrintSwitch<true, bSortCompound>(nRoot, szLevelStart, funcPrint);
		if (bNewLine)
		{
			funcPrint("\n");
		}
	}

	/// @brief 直接序列化，按照一定规则输出为String并返回
	/// @tparam bSortCompound 是否对Compound进行排序
	/// @param nRoot 任意NBT_Type中的类型，仅初始化为视图
	/// @return 返回序列化的结果
	/// @note 注意并非序列化为snbt，一般用于小NBT对象的附加信息输出
	template<bool bSortCompound = true>
	static std::string Serialize(const NBT_Node_View<true> nRoot)
	{
		std::string sRet{};
		SerializeSwitch<true, bSortCompound>(nRoot, sRet);
		return sRet;
	}

#ifdef CJF2_NBT_CPP_USE_XXHASH
	/// @brief 用于插入哈希的示例函数
	/// @param nbtHash 哈希对象，可以向内部添加数据，具体请参考NBT_Hash
	/// @note 此函数为默认实现，请根据需要替换
	static void DefaultFunc(NBT_Hash &nbtHash)
	{
		return;
	}

	/// @brief 函数的类型，用于模板默认值
	using DefaultFuncType = std::decay_t<decltype(DefaultFunc)>;

	/// @brief 对NBT对象进行递归计算哈希
	/// @tparam bSortCompound 是否对Compound进行排序，以获得一致性哈希结果
	/// @tparam TB 开始NBT哈希之前调用的仿函数类型
	/// @tparam TA 结束NBT哈希之后调用的仿函数类型
	/// @param nRoot 任意NBT_Type中的类型，仅初始化为视图
	/// @param nbtHash 哈希对象，使用一个哈希种子初始化，具体请参考NBT_Hash
	/// @param funBefore 开始NBT哈希之前调用的仿函数
	/// @param funAfter 结束NBT哈希之后调用的仿函数
	/// @return 计算的哈希值，可以用于哈希表或比较NBT对象等
	/// @note 注意，递归层数在此函数内没有限制，请注意不要将过深的NBT对象传入导致栈溢出！
	template<bool bSortCompound = true, typename TB = DefaultFuncType, typename TA = DefaultFuncType>//两个函数，分别在前后调用，可以用于插入哈希数据
	static NBT_Hash::HASH_T Hash(const NBT_Node_View<true> nRoot, NBT_Hash nbtHash, TB funBefore = DefaultFunc, TA funAfter = DefaultFunc)
	{
		funBefore(nbtHash);
		HashSwitch<true, bSortCompound>(nRoot, nbtHash);
		funAfter(nbtHash);

		return nbtHash.Digest();

		//调用可行性检测
		static_assert(std::is_invocable_v<TB, decltype(nbtHash)&>, "TB is not a callable object or parameter type mismatch.");
		static_assert(std::is_invocable_v<TA, decltype(nbtHash)&>, "TA is not a callable object or parameter type mismatch.");
	}
#endif

private:
	static auto CompoundSort(const NBT_Type::Compound &cpd)
	{
		std::vector<NBT_Type::Compound::const_iterator> vSort{};
		vSort.reserve(cpd.size());//提前扩容

		//插入迭代器
		for (auto it = cpd.cbegin(), end = cpd.cend(); it != end; ++it)
		{
			vSort.push_back(it);
		}

		//进行排序
		std::sort(vSort.begin(), vSort.end(),
			[](const auto &l, const auto &r) -> bool
			{
				return l->first < r->first;
			}
		);

		return vSort;
	}

	constexpr const static inline char *const LevelPadding = "    ";//默认对齐

	template<typename PrintFunc>
	static void PrintPadding(size_t szLevel, bool bSubLevel, bool bNewLine, PrintFunc &funcPrint)//bSubLevel会让缩进多一层
	{
		if (szLevel == (size_t)-1)//跳过打印
		{
			return;
		}

		if (bNewLine)
		{
			funcPrint("\n");
		}
		
		for (size_t i = 0; i < szLevel; ++i)
		{
			funcPrint("{}", LevelPadding);
		}

		if (bSubLevel)
		{
			funcPrint("{}", LevelPadding);
		}
	}

	template<typename T>
	static void ToHexString(const T &value, std::string &result)
	{
		static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");
		static constexpr char hex_chars[] = "0123456789ABCDEF";

		//按固定字节序处理
		const unsigned char *bytes = (const unsigned char *)&value;
		if constexpr (std::endian::native == std::endian::little)
		{
			for (size_t i = sizeof(T); i-- > 0; )
			{
				result += hex_chars[(bytes[i] >> 4) & 0x0F];//高4
				result += hex_chars[(bytes[i] >> 0) & 0x0F];//低4
			}
		}
		else
		{
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				result += hex_chars[(bytes[i] >> 4) & 0x0F];//高4
				result += hex_chars[(bytes[i] >> 0) & 0x0F];//低4
			}
		}
	}

private:
	//首次调用默认为true，二次调用开始内部主动变为false
	template<bool bRoot = true, bool bSortCompound = true, typename PrintFunc = NBT_Print>//首次使用NBT_Node_View解包，后续直接使用NBT_Node引用免除额外初始化开销
	static void PrintSwitch(std::conditional_t<bRoot, const NBT_Node_View<true> &, const NBT_Node &>nRoot, size_t szLevel, PrintFunc &funcPrint)
	{
		auto tag = nRoot.GetTag();
		switch (tag)
		{
		case NBT_TAG::End:
			{
				funcPrint("[End]");
			}
			break;
		case NBT_TAG::Byte:
			{
				funcPrint("{}B", nRoot.template GetData<NBT_Type::Byte>());
			}
			break;
		case NBT_TAG::Short:
			{
				funcPrint("{}S", nRoot.template GetData<NBT_Type::Short>());
			}
			break;
		case NBT_TAG::Int:
			{
				funcPrint("{}I", nRoot.template GetData<NBT_Type::Int>());
			}
			break;
		case NBT_TAG::Long:
			{
				funcPrint("{}L", nRoot.template GetData<NBT_Type::Long>());
			}
			break;
		case NBT_TAG::Float:
			{
				funcPrint("{}F", nRoot.template GetData<NBT_Type::Float>());
			}
			break;
		case NBT_TAG::Double:
			{
				funcPrint("{}D", nRoot.template GetData<NBT_Type::Double>());
			}
			break;
		case NBT_TAG::ByteArray:
			{
				const auto &arr = nRoot.template GetData<NBT_Type::ByteArray>();
				funcPrint("[B;");
				for (const auto &it : arr)
				{
					funcPrint("{},", it);
				}
				if (arr.size() != 0)
				{
					funcPrint("\b");
				}

				funcPrint("]");
			}
			break;
		case NBT_TAG::IntArray:
			{
				const auto &arr = nRoot.template GetData<NBT_Type::IntArray>();
				funcPrint("[I;");
				for (const auto &it : arr)
				{
					funcPrint("{},", it);
				}

				if (arr.size() != 0)
				{
					funcPrint("\b");
				}
				funcPrint("]");
			}
			break;
		case NBT_TAG::LongArray:
			{
				const auto &arr = nRoot.template GetData<NBT_Type::LongArray>();
				funcPrint("[L;");
				for (const auto &it : arr)
				{
					funcPrint("{},", it);
				}

				if (arr.size() != 0)
				{
					funcPrint("\b");
				}
				funcPrint("]");
			}
			break;
		case NBT_TAG::String:
			{
				funcPrint("\"{}\"", nRoot.template GetData<NBT_Type::String>().ToCharTypeUTF8());
			}
			break;
		case NBT_TAG::List://需要打印缩进的地方
			{
				const auto &list = nRoot.template GetData<NBT_Type::List>();
				PrintPadding(szLevel, false, !bRoot, funcPrint);//不是根部则打印开头换行
				funcPrint("[");
				for (const auto &it : list)
				{
					PrintPadding(szLevel, true, it.GetTag() != NBT_TAG::Compound && it.GetTag() != NBT_TAG::List, funcPrint);
					PrintSwitch<false>(it, szLevel + 1, funcPrint);
					funcPrint(",");
				}

				if (list.Size() != 0)
				{
					funcPrint("\b \b");//清除最后一个逗号
					PrintPadding(szLevel, false, true, funcPrint);//空列表无需换行以及对齐
				}
				funcPrint("]");
			}
			break;
		case NBT_TAG::Compound://需要打印缩进的地方
			{
				const auto &cpd = nRoot.template GetData<NBT_Type::Compound>();
				PrintPadding(szLevel, false, !bRoot, funcPrint);//不是根部则打印开头换行
				funcPrint("{{");//大括号转义

				if constexpr (!bSortCompound)
				{
					for (const auto &it : cpd)
					{
						PrintPadding(szLevel, true, true, funcPrint);
						funcPrint("\"{}\":", it.first.ToCharTypeUTF8());
						PrintSwitch<false>(it.second, szLevel + 1, funcPrint);
						funcPrint(",");
					}
				}
				else
				{
					auto vSort = CompoundSort(cpd);

					for (const auto &it : vSort)
					{
						PrintPadding(szLevel, true, true, funcPrint);
						funcPrint("\"{}\":", it->first.ToCharTypeUTF8());
						PrintSwitch<false>(it->second, szLevel + 1, funcPrint);
						funcPrint(",");
					}
				}

				if (cpd.Size() != 0)
				{
					funcPrint("\b \b");//清除最后一个逗号
					PrintPadding(szLevel, false, true, funcPrint);//空集合无需换行以及对齐
				}
				funcPrint("}}");//大括号转义
			}
			break;
		default:
			{
				funcPrint("[Unknown NBT Tag Type [{:02X}({})]]", (NBT_TAG_RAW_TYPE)tag, (NBT_TAG_RAW_TYPE)tag);
			}
			break;
		}
	}

	//首次调用默认为true，二次调用开始内部主动变为false
	template<bool bRoot = true, bool bSortCompound = true>//首次使用NBT_Node_View解包，后续直接使用NBT_Node引用免除额外初始化开销
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
				ToHexString(nRoot.template GetData<NBT_Type::Byte>(), sRet);
				sRet += 'B';
			}
			break;
		case NBT_TAG::Short:
			{
				ToHexString(nRoot.template GetData<NBT_Type::Short>(), sRet);
				sRet += 'S';
			}
			break;
		case NBT_TAG::Int:
			{
				ToHexString(nRoot.template GetData<NBT_Type::Int>(), sRet);
				sRet += 'I';
			}
			break;
		case NBT_TAG::Long:
			{
				ToHexString(nRoot.template GetData<NBT_Type::Long>(), sRet);
				sRet += 'L';
			}
			break;
		case NBT_TAG::Float:
			{
				ToHexString(nRoot.template GetData<NBT_Type::Float>(), sRet);
				sRet += 'F';
			}
			break;
		case NBT_TAG::Double:
			{
				ToHexString(nRoot.template GetData<NBT_Type::Double>(), sRet);
				sRet += 'D';
			}
			break;
		case NBT_TAG::ByteArray:
			{
				const auto &arr = nRoot.template GetData<NBT_Type::ByteArray>();
				sRet += "[B;";
				for (const auto &it : arr)
				{
					ToHexString(it, sRet);
					sRet += ',';
				}
				if (arr.size() != 0)
				{
					sRet.pop_back();//删掉最后一个逗号
				}
	
				sRet += ']';
			}
			break;
		case NBT_TAG::IntArray:
			{
				const auto &arr = nRoot.template GetData<NBT_Type::IntArray>();
				sRet += "[I;";
				for (const auto &it : arr)
				{
					ToHexString(it, sRet);
					sRet += ',';
				}
				if (arr.size() != 0)
				{
					sRet.pop_back();//删掉最后一个逗号
				}
	
				sRet += ']';
			}
			break;
		case NBT_TAG::LongArray:
			{
				const auto &arr = nRoot.template GetData<NBT_Type::LongArray>();
				sRet += "[L;";
				for (const auto &it : arr)
				{
					ToHexString(it, sRet);
					sRet += ',';
				}
				if (arr.size() != 0)
				{
					sRet.pop_back();//删掉最后一个逗号
				}
	
				sRet += ']';
			}
			break;
		case NBT_TAG::String:
			{
				sRet += '\"';
				sRet += nRoot.template GetData<NBT_Type::String>().ToCharTypeUTF8();
				sRet += '\"';
			}
			break;
		case NBT_TAG::List:
			{
				const auto &list = nRoot.template GetData<NBT_Type::List>();
				sRet += '[';
				for (const auto &it : list)
				{
					SerializeSwitch<false>(it, sRet);
					sRet += ',';
				}
	
				if (list.Size() != 0)
				{
					sRet.pop_back();//删掉最后一个逗号
				}
				sRet += ']';
			}
			break;
		case NBT_TAG::Compound://需要打印缩进的地方
			{
				const auto &cpd = nRoot.template GetData<NBT_Type::Compound>();
				sRet += '{';
	
				if constexpr (!bSortCompound)
				{
					for (const auto &it : cpd)
					{
						sRet += '\"';
						sRet += it.first.ToCharTypeUTF8();
						sRet += "\":";
						SerializeSwitch<false>(it.second, sRet);
						sRet += ',';
					}
				}
				else
				{
					auto vSort = CompoundSort(cpd);

					for (const auto &it : vSort)
					{
						sRet += '\"';
						sRet += it->first.ToCharTypeUTF8();
						sRet += "\":";
						SerializeSwitch<false>(it->second, sRet);
						sRet += ',';
					}
				}
	
				if (cpd.Size() != 0)
				{
					sRet.pop_back();//删掉最后一个逗号
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

#ifdef CJF2_NBT_CPP_USE_XXHASH
	template<bool bRoot = true, bool bSortCompound = true>//首次使用NBT_Node_View解包，后续直接使用NBT_Node引用免除额外初始化开销
	static void HashSwitch(std::conditional_t<bRoot, const NBT_Node_View<true> &, const NBT_Node &>nRoot, NBT_Hash &nbtHash)
	{
		auto tag = nRoot.GetTag();

		//把tag本身作为数据
		{
			const auto &tmp = tag;
			nbtHash.Update(tmp);
		}

		//再读出实际内容作为数据
		switch (tag)
		{
		case NBT_TAG::End:
			{
				//end类型无负载，所以什么也不做
			}
			break;
		case NBT_TAG::Byte:
			{
				const auto &tmp = nRoot.template GetData<NBT_Type::Byte>();
				nbtHash.Update(tmp);
			}
			break;
		case NBT_TAG::Short:
			{
				const auto &tmp = nRoot.template GetData<NBT_Type::Short>();
				nbtHash.Update(tmp);
			}
			break;
		case NBT_TAG::Int:
			{
				const auto &tmp = nRoot.template GetData<NBT_Type::Int>();
				nbtHash.Update(tmp);
			}
			break;
		case NBT_TAG::Long:
			{
				const auto &tmp = nRoot.template GetData<NBT_Type::Long>();
				nbtHash.Update(tmp);
			}
			break;
		case NBT_TAG::Float:
			{
				const auto &tmp = nRoot.template GetData<NBT_Type::Float>();
				nbtHash.Update(tmp);
			}
			break;
		case NBT_TAG::Double:
			{
				const auto &tmp = nRoot.template GetData<NBT_Type::Double>();
				nbtHash.Update(tmp);
			}
			break;
		case NBT_TAG::ByteArray:
			{
				const auto &arr = nRoot.template GetData<NBT_Type::ByteArray>();
				for (const auto &it : arr)
				{
					const auto &tmp = it;
					nbtHash.Update(tmp);
				}
			}
			break;
		case NBT_TAG::IntArray:
			{
				const auto &arr = nRoot.template GetData<NBT_Type::IntArray>();
				for (const auto &it : arr)
				{
					const auto &tmp = it;
					nbtHash.Update(tmp);
				}
			}
			break;
		case NBT_TAG::LongArray:
			{
				const auto &arr = nRoot.template GetData<NBT_Type::LongArray>();
				for (const auto &it : arr)
				{
					const auto &tmp = it;
					nbtHash.Update(tmp);
				}
			}
			break;
		case NBT_TAG::String:
			{
				const auto &tmp = nRoot.template GetData<NBT_Type::String>();
				nbtHash.Update(tmp.data(), tmp.size());
			}
			break;
		case NBT_TAG::List:
			{
				const auto &list = nRoot.template GetData<NBT_Type::List>();
				for (const auto &it : list)
				{
					HashSwitch<false>(it, nbtHash);
				}
			}
			break;
		case NBT_TAG::Compound://需要打印缩进的地方
			{
				const auto &cpd = nRoot.template GetData<NBT_Type::Compound>();

				if constexpr (!bSortCompound)
				{
					for (const auto &it : cpd)
					{
						const auto &tmp = it.first;
						nbtHash.Update(tmp.data(), tmp.size());
						HashSwitch<false>(it.second, nbtHash);
					}
				}
				else//对compound迭代器进行排序，以使得hash获得一致性结果
				{
					auto vSort = CompoundSort(cpd);

					//遍历有序结构
					for (const auto &it : vSort)
					{
						const auto &tmp = it->first;
						nbtHash.Update(tmp.data(), tmp.size());
						HashSwitch<false>(it->second, nbtHash);
					}
				}
			}
			break;
		default:
			{}
			break;
		}
	}
#endif

};
