#pragma once

#include <bit>
#include <concepts>
#include <iterator>
#include <algorithm>

#include "NBT_Print.hpp"//打印输出
#include "NBT_Endian.hpp"
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
	/// @param szPaddingStartLevel 从指定缩进等级开始打印，值为(size_t)-1则不打印缩进
	/// @param strLevelPadding 用于打印一级的空白内容
	/// @param funcPrint 用于输出的仿函数
	template<bool bSortCompound = true, typename PrintFunc = NBT_Print>
	static void Print(const NBT_Node_View<true> nRoot, size_t szPaddingStartLevel = 0, const std::string & strLevelPadding = "    ", PrintFunc funcPrint = NBT_Print{})
	{
		PrintSwitch<true, bSortCompound>(nRoot, szPaddingStartLevel, strLevelPadding, funcPrint);
	}

	/// @brief 直接序列化，按照一定规则输出为String并返回
	/// @tparam bSortCompound 是否对Compound进行排序
	/// @tparam bHexNumType 是否使用十六进制无损输出值
	/// @tparam bSnbtType 是否对输出为SNBT格式（SNBT下强制为十进制值，忽略bHexNumType参数）
	/// @param nRoot 任意NBT_Type中的类型，仅初始化为视图
	/// @return 返回序列化的结果
	/// @note 注意并非序列化为snbt，一般用于小NBT对象的附加信息输出
	template<bool bSortCompound = true, bool bHexNumType = true, bool bSnbtType = false>
	static std::conditional_t<bSnbtType, NBT_Type::String, std::string> Serialize(const NBT_Node_View<true> nRoot)
	{
		std::conditional_t<bSnbtType, NBT_Type::String, std::string> sRet{};
		SerializeSwitch<true, bSortCompound, bHexNumType, bSnbtType>(nRoot, sRet);
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

protected:
///@cond
	template<typename PrintFunc>
	static void PrintPadding(size_t szLevel, bool bSubLevel, bool bNewLine, const std::string &strLevelPadding, PrintFunc &funcPrint)//bSubLevel会让缩进多一层
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
			funcPrint("{}", strLevelPadding);
		}

		if (bSubLevel)
		{
			funcPrint("{}", strLevelPadding);
		}
	}

	template<typename T>
	requires(std::is_arithmetic_v<T>)
	static void NumericToHexString(const T &value, std::string &result)
	{
		//映射
		static constexpr char hex_chars[] = "0123456789ABCDEF";

		//前缀
		result += "0x";

		//按照原始字节序处理
		using Raw_T = decltype([](void) -> auto
		{
			if constexpr (NBT_Type::IsNumericType_V<T>)
			{
				return NBT_Type::BuiltinRawType_T<T>{};
			}
			else
			{
				return T{};
			}
		}());
		Raw_T uintVal = std::bit_cast<Raw_T>(value);

		for (size_t i = 0; i < sizeof(T); ++i)
		{
			uint8_t u8Byte = (uintVal >> 8 * (sizeof(T) - i - 1)) & 0xFF;//遍历字节，从高到低

			result += hex_chars[(u8Byte >> 4) & 0x0F];//高4
			result += hex_chars[(u8Byte >> 0) & 0x0F];//低4
		}
	}

	template<typename T, typename STR_T>
	requires(NBT_Type::IsNumericType_V<T> && (std::is_same_v<STR_T, NBT_Type::String> || std::is_same_v<STR_T, std::string>))
	static void NumericToDecString(const T &value, STR_T &result)
	{
		std::string tmp{};
		if constexpr (NBT_Type::IsFloatingType_V<T>)
		{
			tmp = std::format("{:.{}g}", value, std::numeric_limits<T>::max_digits10);
		}
		else if constexpr (NBT_Type::IsIntegerType_V<T>)
		{
			tmp = std::format("{:d}", value);
		}
		else
		{
			static_assert(false,"T type unknown");
		}

		if constexpr (std::is_same_v<STR_T, NBT_Type::String>)
		{
			result += (NBT_Type::String)tmp;//转换为NBT字符串
		}
		else if constexpr(std::is_same_v<STR_T, std::string>)
		{
			result += tmp;
		}
		else
		{
			static_assert(false, "STR_T type unknown");
		}
	}
///@endcond

protected:
///@cond
	//首次调用默认为true，二次调用开始内部主动变为false
	template<bool bRoot, bool bSortCompound, typename PrintFunc = NBT_Print>//首次使用NBT_Node_View解包，后续直接使用NBT_Node引用免除额外初始化开销
	static void PrintSwitch(std::conditional_t<bRoot, const NBT_Node_View<true> &, const NBT_Node &>nRoot, size_t szLevel, const std::string &strLevelPadding, PrintFunc &funcPrint)
	{
		static auto PrintArray = [](const std::string strBeg, const auto &vArr, const std::string strEnd, PrintFunc &funcPrint) -> void
		{
			funcPrint("{}", strBeg);
			bool bFirst = true;
			for (const auto &it : vArr)
			{
				if (bFirst)
				{
					bFirst = false;
					funcPrint("{}", it);
				}
				funcPrint(",{}", it);
			}
			funcPrint("{}", strEnd);
		};


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
				funcPrint("{}B", nRoot.template Get<NBT_Type::Byte>());
			}
			break;
		case NBT_TAG::Short:
			{
				funcPrint("{}S", nRoot.template Get<NBT_Type::Short>());
			}
			break;
		case NBT_TAG::Int:
			{
				funcPrint("{}I", nRoot.template Get<NBT_Type::Int>());
			}
			break;
		case NBT_TAG::Long:
			{
				funcPrint("{}L", nRoot.template Get<NBT_Type::Long>());
			}
			break;
		case NBT_TAG::Float:
			{
				funcPrint("{}F", nRoot.template Get<NBT_Type::Float>());
			}
			break;
		case NBT_TAG::Double:
			{
				funcPrint("{}D", nRoot.template Get<NBT_Type::Double>());
			}
			break;
		case NBT_TAG::ByteArray:
			{
				const auto &arr = nRoot.template Get<NBT_Type::ByteArray>();
				PrintArray("[B;", arr, "]", funcPrint);
			}
			break;
		case NBT_TAG::IntArray:
			{
				const auto &arr = nRoot.template Get<NBT_Type::IntArray>();
				PrintArray("[I;", arr, "]", funcPrint);
			}
			break;
		case NBT_TAG::LongArray:
			{
				const auto &arr = nRoot.template Get<NBT_Type::LongArray>();
				PrintArray("[L;", arr, "]", funcPrint);
			}
			break;
		case NBT_TAG::String:
			{
				funcPrint("\"{}\"", nRoot.template Get<NBT_Type::String>().ToCharTypeUTF8());
			}
			break;
		case NBT_TAG::List://需要打印缩进的地方
			{
				const auto &list = nRoot.template Get<NBT_Type::List>();
				PrintPadding(szLevel, false, !bRoot, strLevelPadding, funcPrint);//不是根部则打印开头换行
				funcPrint("[");

				bool bFirst = true;
				for (const auto &it : list)
				{
					if (bFirst)
					{
						bFirst = false;
					}
					else
					{
						funcPrint(",");
					}

					PrintPadding(szLevel, true, it.GetTag() != NBT_TAG::Compound && it.GetTag() != NBT_TAG::List, strLevelPadding, funcPrint);
					PrintSwitch<false, bSortCompound, PrintFunc>(it, szLevel + 1, strLevelPadding, funcPrint);
				}

				if (list.Size() != 0)//空列表无需换行以及对齐
				{
					PrintPadding(szLevel, false, true, strLevelPadding, funcPrint);
				}

				funcPrint("]");
			}
			break;
		case NBT_TAG::Compound://需要打印缩进的地方
			{
				const auto &cpd = nRoot.template Get<NBT_Type::Compound>();
				PrintPadding(szLevel, false, !bRoot, strLevelPadding, funcPrint);//不是根部则打印开头换行
				funcPrint("{{");//大括号转义

				if constexpr (!bSortCompound)
				{
					bool bFirst = true;
					for (const auto &it : cpd)
					{
						if (bFirst)
						{
							bFirst = false;
						}
						else
						{
							funcPrint(",");
						}

						PrintPadding(szLevel, true, true, strLevelPadding, funcPrint);
						funcPrint("\"{}\":", it.first.ToCharTypeUTF8());
						PrintSwitch<false, bSortCompound, PrintFunc>(it.second, szLevel + 1, strLevelPadding, funcPrint);
					}
				}
				else
				{
					auto vSort = cpd.KeySortIt();

					bool bFirst = true;
					for (const auto &it : vSort)
					{
						if (bFirst)
						{
							bFirst = false;
						}
						else
						{
							funcPrint(",");
						}

						PrintPadding(szLevel, true, true, strLevelPadding, funcPrint);
						funcPrint("\"{}\":", it->first.ToCharTypeUTF8());
						PrintSwitch<false, bSortCompound, PrintFunc>(it->second, szLevel + 1, strLevelPadding, funcPrint);
					}
				}

				if (cpd.Size() != 0)//空集合无需换行以及对齐
				{
					PrintPadding(szLevel, false, true, strLevelPadding, funcPrint);
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
	template<bool bRoot, bool bSortCompound, bool bHexNumType, bool bSnbtType>//首次使用NBT_Node_View解包，后续直接使用NBT_Node引用免除额外初始化开销
	static void SerializeSwitch(std::conditional_t<bRoot, const NBT_Node_View<true> &, const NBT_Node &>nRoot, std::conditional_t<bSnbtType, NBT_Type::String, std::string> &sRet)
	{
		auto tag = nRoot.GetTag();
		switch (tag)
		{
		case NBT_TAG::End:
			{
				if constexpr (bSnbtType)
				{
					sRet += MU8STR("[End]");
				}
				else
				{
					sRet += "[End]";
				}
			}
			break;
		case NBT_TAG::Byte:
			{
				if constexpr (bSnbtType || !bHexNumType)//snbt必须是dec
				{
					NumericToDecString(nRoot.template Get<NBT_Type::Byte>(), sRet);
				}
				else
				{
					NumericToHexString(nRoot.template Get<NBT_Type::Byte>(), sRet);
				}
				sRet += 'B';
			}
			break;
		case NBT_TAG::Short:
			{
				if constexpr (bSnbtType || !bHexNumType)
				{
					NumericToDecString(nRoot.template Get<NBT_Type::Short>(), sRet);
				}
				else
				{
					NumericToHexString(nRoot.template Get<NBT_Type::Short>(), sRet);
				}
				sRet += 'S';
			}
			break;
		case NBT_TAG::Int:
			{
				if constexpr (bSnbtType || !bHexNumType)
				{
					NumericToDecString(nRoot.template Get<NBT_Type::Int>(), sRet);
				}
				else
				{
					NumericToHexString(nRoot.template Get<NBT_Type::Int>(), sRet);
				}
				sRet += 'I';
			}
			break;
		case NBT_TAG::Long:
			{
				if constexpr (bSnbtType || !bHexNumType)
				{
					NumericToDecString(nRoot.template Get<NBT_Type::Long>(), sRet);
				}
				else
				{
					NumericToHexString(nRoot.template Get<NBT_Type::Long>(), sRet);
				}
				sRet += 'L';
			}
			break;
		case NBT_TAG::Float:
			{
				if constexpr (bSnbtType || !bHexNumType)
				{
					NumericToDecString(nRoot.template Get<NBT_Type::Float>(), sRet);
				}
				else
				{
					NumericToHexString(nRoot.template Get<NBT_Type::Float>(), sRet);
				}
				sRet += 'F';
			}
			break;
		case NBT_TAG::Double:
			{
				if constexpr (bSnbtType || !bHexNumType)
				{
					NumericToDecString(nRoot.template Get<NBT_Type::Double>(), sRet);
				}
				else
				{
					NumericToHexString(nRoot.template Get<NBT_Type::Double>(), sRet);
				}
				sRet += 'D';
			}
			break;
		case NBT_TAG::ByteArray:
			{
				const auto &arr = nRoot.template Get<NBT_Type::ByteArray>();

				if constexpr (bSnbtType)
				{
					sRet += MU8STR("[B;");
				}
				else
				{
					sRet += "[B;";
				}
				
				for (const auto &it : arr)
				{
					if constexpr (bSnbtType || !bHexNumType)
					{
						NumericToDecString(it, sRet);
					}
					else
					{
						NumericToHexString(it, sRet);
					}
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
				const auto &arr = nRoot.template Get<NBT_Type::IntArray>();
				
				if constexpr (bSnbtType)
				{
					sRet += MU8STR("[I;");
				}
				else
				{
					sRet += "[I;";
				}

				for (const auto &it : arr)
				{
					if constexpr (bSnbtType || !bHexNumType)
					{
						NumericToDecString(it, sRet);
					}
					else
					{
						NumericToHexString(it, sRet);
					}
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
				const auto &arr = nRoot.template Get<NBT_Type::LongArray>();
				
				if constexpr (bSnbtType)
				{
					sRet += MU8STR("[L;");
				}
				else
				{
					sRet += "[L;";
				}

				for (const auto &it : arr)
				{
					if constexpr (bSnbtType || !bHexNumType)
					{
						NumericToDecString(it, sRet);
					}
					else
					{
						NumericToHexString(it, sRet);
					}
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
				if constexpr (bSnbtType)
				{
					sRet += nRoot.template Get<NBT_Type::String>();
				}
				else
				{
					sRet += nRoot.template Get<NBT_Type::String>().ToCharTypeUTF8();
				}
				sRet += '\"';
			}
			break;
		case NBT_TAG::List:
			{
				const auto &list = nRoot.template Get<NBT_Type::List>();
				sRet += '[';
				for (const auto &it : list)
				{
					SerializeSwitch<false, bSortCompound, bHexNumType, bSnbtType>(it, sRet);
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
				const auto &cpd = nRoot.template Get<NBT_Type::Compound>();
				sRet += '{';
	
				if constexpr (!bSortCompound)
				{
					for (const auto &it : cpd)
					{
						sRet += '\"';
						if constexpr (bSnbtType)
						{
							sRet += it.first;
							sRet += MU8STR("\":");
						}
						else
						{
							sRet += it.first.ToCharTypeUTF8();
							sRet += "\":";
						}
						SerializeSwitch<false, bSortCompound, bHexNumType, bSnbtType>(it.second, sRet);
						sRet += ',';
					}
				}
				else
				{
					auto vSort = cpd.KeySortIt();

					for (const auto &it : vSort)
					{
						sRet += '\"';
						if constexpr (bSnbtType)
						{
							sRet += it->first;
							sRet += MU8STR("\":");
						}
						else
						{
							sRet += it->first.ToCharTypeUTF8();
							sRet += "\":";
						}
						SerializeSwitch<false, bSortCompound, bHexNumType, bSnbtType>(it->second, sRet);
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
				if constexpr (bSnbtType)
				{
					//Ignore Error
				}
				else
				{
					sRet += "[Unknown NBT Tag Type [";
					NumericToHexString((NBT_TAG_RAW_TYPE)tag, sRet);
					sRet += "]]";
				}
			}
			break;
		}
	}

#ifdef CJF2_NBT_CPP_USE_XXHASH
	template<bool bRoot, bool bSortCompound>//首次使用NBT_Node_View解包，后续直接使用NBT_Node引用免除额外初始化开销
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
				const auto &tmp = nRoot.template Get<NBT_Type::Byte>();
				nbtHash.Update(tmp);
			}
			break;
		case NBT_TAG::Short:
			{
				const auto &tmp = nRoot.template Get<NBT_Type::Short>();
				nbtHash.Update(tmp);
			}
			break;
		case NBT_TAG::Int:
			{
				const auto &tmp = nRoot.template Get<NBT_Type::Int>();
				nbtHash.Update(tmp);
			}
			break;
		case NBT_TAG::Long:
			{
				const auto &tmp = nRoot.template Get<NBT_Type::Long>();
				nbtHash.Update(tmp);
			}
			break;
		case NBT_TAG::Float:
			{
				const auto &tmp = nRoot.template Get<NBT_Type::Float>();
				nbtHash.Update(tmp);
			}
			break;
		case NBT_TAG::Double:
			{
				const auto &tmp = nRoot.template Get<NBT_Type::Double>();
				nbtHash.Update(tmp);
			}
			break;
		case NBT_TAG::ByteArray:
			{
				const auto &arr = nRoot.template Get<NBT_Type::ByteArray>();
				for (const auto &it : arr)
				{
					const auto &tmp = it;
					nbtHash.Update(tmp);
				}
			}
			break;
		case NBT_TAG::IntArray:
			{
				const auto &arr = nRoot.template Get<NBT_Type::IntArray>();
				for (const auto &it : arr)
				{
					const auto &tmp = it;
					nbtHash.Update(tmp);
				}
			}
			break;
		case NBT_TAG::LongArray:
			{
				const auto &arr = nRoot.template Get<NBT_Type::LongArray>();
				for (const auto &it : arr)
				{
					const auto &tmp = it;
					nbtHash.Update(tmp);
				}
			}
			break;
		case NBT_TAG::String:
			{
				const auto &tmp = nRoot.template Get<NBT_Type::String>();
				nbtHash.Update(tmp.data(), tmp.size());
			}
			break;
		case NBT_TAG::List:
			{
				const auto &list = nRoot.template Get<NBT_Type::List>();
				for (const auto &it : list)
				{
					HashSwitch<false, bSortCompound>(it, nbtHash);
				}
			}
			break;
		case NBT_TAG::Compound://需要打印缩进的地方
			{
				const auto &cpd = nRoot.template Get<NBT_Type::Compound>();

				if constexpr (!bSortCompound)
				{
					for (const auto &it : cpd)
					{
						const auto &tmp = it.first;
						nbtHash.Update(tmp.data(), tmp.size());
						HashSwitch<false, bSortCompound>(it.second, nbtHash);
					}
				}
				else//对compound迭代器进行排序，以使得hash获得一致性结果
				{
					auto vSort = cpd.KeySortIt();

					//遍历有序结构
					for (const auto &it : vSort)
					{
						const auto &tmp = it->first;
						nbtHash.Update(tmp.data(), tmp.size());
						HashSwitch<false, bSortCompound>(it->second, nbtHash);
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
///@endcond

};
