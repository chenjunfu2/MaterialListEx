#pragma once

#include "NBT_Node.hpp"
#include "NBT_Print.hpp"

#include <stdint.h>
#include <vector>

/// @file
/// @brief NBT类型二进制流访问器（作为扫描工具的回调）

/// @brief 提示性实现类（鸭子类型），仅用于模板通过性验证与用户接口提示
/// @note 用户自定义的访问器类需要实现与此类相同的成员函数（不必继承），
/// 并满足 IsLookLike_NBT_Visitor 概念。
class NBT_Visitor
{
public:
	/// @brief 控制流返回码（用于普通回调）
	enum class ResultControl : uint8_t
	{
		Continue,	///< 继续处理（继续迭代）
		Break,		///< 跳过剩余值（离开当前结构层级回到父层级）
		Stop,		///< 停止处理（终止解析）
	};

	/// @brief 控制流返回码（用于嵌套结构回调）
	enum class NestingControl : uint8_t
	{
		Enter,	///< 进入当前值（递归进入嵌套结构或展开值）
		Skip,	///< 跳过当前值（跳过递归进入嵌套结构或展开值）
		Break,	///< 跳过剩余值（离开当前结构层级回到父层级）
		Stop,	///< 停止处理（终止解析）
	};

public:
	/// @brief 处理数值类型节点
	/// @tparam T 数值类型，必须满足 NBT_Type::IsNumericType_V
	/// @param tNumericResult 数值结果
	/// @return 控制码，决定后续行为
	template<typename T>
	requires(NBT_Type::IsNumericType_V<T>)
	ResultControl VisitNumericResult(T tNumericResult)
	{
		//do something...
		return ResultControl::Continue;
	}

	/// @brief 处理数组类型节点（ByteArray/IntArray/LongArray）
	/// @tparam T 数组类型，必须满足 NBT_Type::IsArrayType_V 且不是引用
	/// @param tArrayResult 数组结果（右值引用）
	/// @return 控制码，决定后续行为
	template<typename T>
	requires(NBT_Type::IsArrayType_V<T> && !std::is_reference_v<T>)//防止引用折叠
	ResultControl VisitArrayResult(T &&tArrayResult)
	{
		//do something...
		return ResultControl::Continue;
	}

	/// @brief 处理字符串类型节点
	/// @param strResult 字符串结果（右值引用）
	/// @return 控制码，决定后续行为
	ResultControl VisitStringResult(NBT_Type::String &&strResult)
	{
		//do something...
		return ResultControl::Continue;
	}

	/// @brief 处理 End 标记节点
	/// @note 通常用于 List 中的空列表或标记结束，一般用户无需特殊处理
	/// @return 控制码，决定后续行为
	ResultControl VisitEndResult(void)
	{
		//do something...
		return ResultControl::Continue;
	}

	/// @brief 开始处理一个 List 节点
	/// @param enListElementTag 列表元素的类型标签
	/// @param szListLength 列表长度
	/// @return 控制码，决定后续行为
	ResultControl VisitListBegin(NBT_TAG enListElementTag, size_t szListLength)
	{
		//do something...
		return ResultControl::Continue;
	}

	/// @brief 开始处理 List 中的一个元素
	/// @param enListElementTag 列表元素的类型标签
	/// @param szListIndex 当前元素在列表中的索引（从0开始）
	/// @return 嵌套控制码，决定是否进入该元素、跳过或停止
	NestingControl VisitListElementBegin(NBT_TAG enListElementTag, size_t szListIndex)
	{
		//do something...
		return NestingControl::Enter;
	}

	/// @brief 结束处理 List 中的一个元素
	/// @param enListElementTag 列表元素的类型标签
	/// @param szListIndex 当前元素在列表中的索引（从0开始）
	/// @return 控制码，决定后续行为
	ResultControl VisitListElementEnd(NBT_TAG enListElementTag, size_t szListIndex)
	{
		//do something...
		return ResultControl::Continue;
	}

	/// @brief 结束处理整个 List 节点
	/// @return 控制码，决定后续行为
	ResultControl VisitListEnd(void)
	{
		//do something...
		return ResultControl::Continue;
	}

	/// @brief 开始处理一个 Compound 节点
	/// @return 控制码，决定后续行为
	ResultControl VisitCompoundBegin(void)
	{
		//do something...
		return ResultControl::Continue;
	}

	/// @brief 在 Compound 中，开始处理下一个条目之前，仅提供类型信息
	/// @param enCompoundEntryTag 条目的类型标签
	/// @return 嵌套控制码，决定后续行为
	NestingControl VisitCompoundNextEntryType(NBT_TAG enCompoundEntryTag)
	{
		//do something...
		return NestingControl::Enter;
	}

	/// @brief 开始处理 Compound 中的一个条目（键值对）
	/// @param enCompoundEntryTag 条目的类型标签
	/// @param sName 条目的键名（右值引用）
	/// @return 嵌套控制码，决定后续行为
	NestingControl VisitCompoundEntryBegin(NBT_TAG enCompoundEntryTag, NBT_Type::String &&sName)
	{
		//do something...
		return NestingControl::Enter;
	}

	/// @brief 结束处理 Compound 中的一个条目
	/// @param enCompoundEntryTag 条目的类型标签
	/// @param sName 条目的键名（右值引用）
	/// @return 控制码，决定后续行为
	ResultControl VisitCompoundEntryEnd(NBT_TAG enCompoundEntryTag, NBT_Type::String &&sName)
	{
		//do something...
		return ResultControl::Continue;
	}

	/// @brief 结束处理整个 Compound 节点
	/// @return 控制码，决定后续行为
	ResultControl VisitCompoundEnd(void)
	{
		//do something...
		return ResultControl::Continue;
	}

	/// @brief 整个 NBT 扫描开始回调（根部调用）
	void VisitBegin(void)
	{
		//do something...
		return;
	}

	/// @brief 整个 NBT 扫描结束回调（根部调用）
	void VisitEnd(void)
	{
		//do something...
		return;
	}

	/// @brief 错误处理回调
	/// @tparam Args 格式化参数类型包
	/// @param lvl 错误级别
	/// @param fmt 格式化字符串
	/// @param args 格式化参数
	/// @note 该函数仅用于输出/记录错误信息，不影响扫描的控制流。
	template<typename... Args>
	void VisitError(NBT_Print_Level lvl, const std::format_string<Args...> fmt, Args&&... args) noexcept
	{
		//throw or print error
		return;
	}
};

/// @brief 检查类型是否符合 NBT 访问器（Visitor）的接口要求
/// @tparam T 待检查的类型
/// @note 该概念要求类型 T 实现 NBT_Visitor 类中定义的所有公共成员函数（包括数值/数组/字符串访问、
/// List/Compound 相关回调、错误处理等）。满足此概念的类型可作为 NBT_Scanner::ScanNBT 的访问器参数。
/// @see NBT_Visitor 提示性实现类，展示了完整的接口原型。
template <typename T>
concept IsLookLike_NBT_Visitor =
requires(
	T visitor,
	NBT_Visitor nbt_visitor,

	NBT_Type::Byte nbt_byte,
	NBT_Type::Short nbt_short,
	NBT_Type::Int nbt_int,
	NBT_Type::Long nbt_long,
	NBT_Type::Float nbt_float,
	NBT_Type::Double nbt_double,

	NBT_TAG nbt_tag,
	size_t nbt_list_length,
	size_t nbt_list_index,

	NBT_Type::ByteArray nbt_bytearray,
	NBT_Type::IntArray nbt_intarray,
	NBT_Type::LongArray nbt_longarray,
	NBT_Type::String nbt_string,
	NBT_Print_Level nbt_print_level
	)
{
	//数值类型访问方法
	{
		visitor.VisitNumericResult(nbt_byte)
	} -> std::same_as<decltype(nbt_visitor.VisitNumericResult(nbt_byte))>;
	{
		visitor.VisitNumericResult(nbt_short)
	} -> std::same_as<decltype(nbt_visitor.VisitNumericResult(nbt_short))>;
	{
		visitor.VisitNumericResult(nbt_int)
	} -> std::same_as<decltype(nbt_visitor.VisitNumericResult(nbt_int))>;
	{
		visitor.VisitNumericResult(nbt_long)
	} -> std::same_as<decltype(nbt_visitor.VisitNumericResult(nbt_long))>;
	{
		visitor.VisitNumericResult(nbt_float)
	} -> std::same_as<decltype(nbt_visitor.VisitNumericResult(nbt_float))>;
	{
		visitor.VisitNumericResult(nbt_double)
	} -> std::same_as<decltype(nbt_visitor.VisitNumericResult(nbt_double))>;

	//数组类型访问方法
	{
		visitor.VisitArrayResult(std::move(nbt_bytearray))
	} -> std::same_as<decltype(nbt_visitor.VisitArrayResult(std::move(nbt_bytearray)))>;
	{
		visitor.VisitArrayResult(std::move(nbt_intarray))
	} -> std::same_as<decltype(nbt_visitor.VisitArrayResult(std::move(nbt_intarray)))>;
	{
		visitor.VisitArrayResult(std::move(nbt_longarray))
	} -> std::same_as<decltype(nbt_visitor.VisitArrayResult(std::move(nbt_longarray)))>;

	//字符串访问方法
	{
		visitor.VisitStringResult(std::move(nbt_string))
	} -> std::same_as<decltype(nbt_visitor.VisitStringResult(std::move(nbt_string)))>;

	//结束标记访问方法
	{
		visitor.VisitEndResult()
	} -> std::same_as<decltype(nbt_visitor.VisitEndResult())>;


	//List相关方法
	{
		visitor.VisitListBegin(nbt_tag, nbt_list_length)
	} -> std::same_as<decltype(nbt_visitor.VisitListBegin(nbt_tag, nbt_list_length))>;
	{
		visitor.VisitListElementBegin(nbt_tag, nbt_list_index)
	} -> std::same_as<decltype(nbt_visitor.VisitListElementBegin(nbt_tag, nbt_list_index))>;
	{
		visitor.VisitListElementEnd(nbt_tag, nbt_list_index)
	} -> std::same_as<decltype(nbt_visitor.VisitListElementEnd(nbt_tag, nbt_list_index))>;
	{
		visitor.VisitListEnd()
	} -> std::same_as<decltype(nbt_visitor.VisitListEnd())>;

	//Compound相关方法
	{
		visitor.VisitCompoundBegin()
	} -> std::same_as<decltype(nbt_visitor.VisitCompoundBegin())>;
	{
		visitor.VisitCompoundNextEntryType(nbt_tag)
	} -> std::same_as<decltype(nbt_visitor.VisitCompoundNextEntryType(nbt_tag))>;
	{
		visitor.VisitCompoundEntryBegin(nbt_tag, std::move(nbt_string))
	} -> std::same_as<decltype(nbt_visitor.VisitCompoundEntryBegin(nbt_tag, std::move(nbt_string)))>;
	{
		visitor.VisitCompoundEntryEnd(nbt_tag, std::move(nbt_string))
	} -> std::same_as<decltype(nbt_visitor.VisitCompoundEntryEnd(nbt_tag, std::move(nbt_string)))>;
	{
		visitor.VisitCompoundEnd()
	} -> std::same_as<decltype(nbt_visitor.VisitCompoundEnd())>;

	//开始/结束方法
	{
		visitor.VisitBegin()
	};
	{
		visitor.VisitEnd()
	};

	//错误处理（不完全验证）
	{
		visitor.VisitError(nbt_print_level, "error message")
	};
	{
		visitor.VisitError(nbt_print_level, "error with code: {}", 0)
	};
	{
		visitor.VisitError(nbt_print_level, "error with info: {}", "test info")
	};
};

static_assert(IsLookLike_NBT_Visitor<NBT_Visitor>);

/// @brief NBT 数据收集器，实现访问器接口，将扫描结果构建为完整的 Compound 树
/// @note 该类会自动管理栈帧，将解析出的值插入到正确的父容器（Compound 或 List）中。
/// 支持多次调用 ScanNBT 合并多个 NBT 流（不会自动清除已有根节点）。
class NBT_Visitor_Collector
{
protected:
	/// @brief 栈帧
	struct Frame
	{
		/// @brief 帧的容器类型
		enum class Type : uint8_t
		{
			Compound,	///< 当前存储的是 Compound 类型
			List,		///< 当前存储的是 List 类型
		};

	public:
		Type enType;	///< 当前帧的容器类型
		union
		{
			NBT_Type::Compound *pCompound;	///< 当前存储的 Compound 容器指针
			NBT_Type::List *pList;			///< 当前存储的 List 容器指针
		};
	};

protected:
	NBT_Type::String sPendingKey;	///< 暂存的键名，用于插入 Compound
	std::vector<Frame> vStack{};	///< 栈，记录当前嵌套的容器
	NBT_Type::Compound cpdRoot{};	///< 根 Compound 节点

protected:
	/// @brief 将值追加到当前栈顶的容器中
	/// @tparam T 值类型
	/// @param tVal 待插入的值
	/// @return 成功返回 true，失败（栈空或容器无效）返回 false
	/// @note 若值本身是 Compound 或 List，会自动推入新栈帧。
	template<typename T>
	bool AppendStackTop(T &&tVal)
	{
		if (vStack.empty())
		{
			return false;
		}

		void *pNewElement = NULL;

		Frame &stTopFrame = vStack.back();
		switch (stTopFrame.enType)
		{
		case Frame::Type::Compound:
			{
				if (stTopFrame.pCompound == NULL)
				{
					return false;
				}

				//尝试插入，遇到重复则替换
				auto [it, b] = stTopFrame.pCompound->Put(std::move(sPendingKey), std::forward<T>(tVal));
				pNewElement = (void*)&(it->second);
			}
			break;
		case Frame::Type::List:
			{
				if (stTopFrame.pList == NULL)
				{
					return false;
				}

				pNewElement = (void*)&stTopFrame.pList->AddBack(std::forward<T>(tVal));
			}
			break;
		default:
			return false;
			break;
		}

		if constexpr (NBT_Type::IsCompoundType_V<T>)
		{
			vStack.push_back(
				Frame
				{
					.enType = Frame::Type::Compound,
					.pCompound = (NBT_Type::Compound *)pNewElement,
				}
			);
		}
		else if constexpr (NBT_Type::IsListType_V<T>)
		{
			vStack.push_back(
				Frame
				{
					.enType = Frame::Type::List,
					.pList = (NBT_Type::List *)pNewElement,
				}
			);
		}

		return true;
	}

	/// @brief 弹出栈顶，并检查类型是否匹配
	/// @param enTypeRequires 要求的栈帧类型
	/// @return 成功（栈非空且类型匹配）返回 true，否则返回 false
	bool PopStack(Frame::Type enTypeRequires)
	{
		if (vStack.empty())
		{
			return false;
		}

		if (vStack.back().enType != enTypeRequires)
		{
			return false;
		}

		vStack.pop_back();
		return true;
	}

public:
	NBT_Visitor_Collector(void) = default;
	~NBT_Visitor_Collector(void) = default;

	/// @brief 禁用拷贝构造函数
	NBT_Visitor_Collector(const NBT_Visitor_Collector &) = delete;
	/// @brief 默认移动构造函数
	NBT_Visitor_Collector(NBT_Visitor_Collector &&) = default;
	/// @brief 禁用拷贝赋值运算符
	NBT_Visitor_Collector &operator=(const NBT_Visitor_Collector &) = delete;
	/// @brief 默认移动赋值运算符
	NBT_Visitor_Collector &operator=(NBT_Visitor_Collector &&) = default;

public:
	using ResultControl = NBT_Visitor::ResultControl; ///< 控制流返回码（普通回调），直接映射 NBT_Visitor::ResultControl
	using NestingControl = NBT_Visitor::NestingControl;	///< 控制流返回码（嵌套结构入口回调），直接映射 NBT_Visitor::NestingControl

	/// @brief 获取根节点的常量引用
	/// @return 根 Compound 对象的常量引用
	const NBT_Type::Compound &ViewRoot(void) const noexcept
	{
		return cpdRoot;
	}

	/// @brief 移动根节点（获取所有权）
	/// @return 根 Compound 对象的右值引用
	NBT_Type::Compound &&MoveRoot(void) noexcept
	{
		return std::move(cpdRoot);
	}

public:
	/// @copydoc NBT_Visitor::VisitNumericResult
	template<typename T>
	requires(NBT_Type::IsNumericType_V<T>)
	ResultControl VisitNumericResult(T tNumericResult)
	{
		if (!AppendStackTop(tNumericResult))
		{
			return ResultControl::Stop;
		}
		return ResultControl::Continue;
	}

	/// @copydoc NBT_Visitor::VisitArrayResult
	template<typename T>
	requires(NBT_Type::IsArrayType_V<T> && !std::is_reference_v<T>)//防止引用折叠
	ResultControl VisitArrayResult(T &&tArrayResult)
	{
		if (!AppendStackTop(tArrayResult))
		{
			return ResultControl::Stop;
		}
		return ResultControl::Continue;
	}

	/// @copydoc NBT_Visitor::VisitStringResult
	ResultControl VisitStringResult(NBT_Type::String &&strResult)
	{
		if (!AppendStackTop(strResult))
		{
			return ResultControl::Stop;
		}
		return ResultControl::Continue;
	}

	/// @copydoc NBT_Visitor::VisitEndResult
	ResultControl VisitEndResult(void)
	{
		if (!AppendStackTop(NBT_Type::End{}))
		{
			return ResultControl::Stop;
		}
		return ResultControl::Continue;
	}

	/// @copydetails NBT_Visitor::VisitListBegin
	ResultControl VisitListBegin(NBT_TAG enListElementTag, size_t szListLength)
	{
		NBT_Type::List tmpList{};
		tmpList.Reserve(szListLength);

		if (!AppendStackTop(std::move(tmpList)))
		{
			return ResultControl::Stop;
		}
		return ResultControl::Continue;
	}

	/// @copydoc NBT_Visitor::VisitListElementBegin
	NestingControl VisitListElementBegin(NBT_TAG enListElementTag, size_t szListIndex)
	{
		return NestingControl::Enter;
	}

	/// @copydoc NBT_Visitor::VisitListElementEnd
	ResultControl VisitListElementEnd(NBT_TAG enListElementTag, size_t szListIndex)
	{
		return ResultControl::Continue;
	}

	/// @copydetails NBT_Visitor::VisitListEnd
	ResultControl VisitListEnd(void)
	{
		if (!PopStack(Frame::Type::List))
		{
			return ResultControl::Stop;
		}
		return ResultControl::Continue;
	}

	/// @copydetails NBT_Visitor::VisitCompoundBegin
	ResultControl VisitCompoundBegin(void)
	{
		if (!AppendStackTop(NBT_Type::Compound{}))
		{
			return ResultControl::Stop;
		}
		return ResultControl::Continue;
	}

	/// @copydoc NBT_Visitor::VisitCompoundNextEntryType
	NestingControl VisitCompoundNextEntryType(NBT_TAG enCompoundEntryTag)
	{
		return NestingControl::Enter;
	}

	/// @copydetails NBT_Visitor::VisitCompoundEntryBegin
	NestingControl VisitCompoundEntryBegin(NBT_TAG enCompoundEntryTag, NBT_Type::String &&sName)
	{
		sPendingKey = sName;
		return NestingControl::Enter;
	}

	/// @copydoc NBT_Visitor::VisitCompoundEntryEnd
	ResultControl VisitCompoundEntryEnd(NBT_TAG enCompoundEntryTag, NBT_Type::String &&sName)
	{
		return ResultControl::Continue;
	}

	/// @copydetails NBT_Visitor::VisitCompoundEnd
	ResultControl VisitCompoundEnd(void)
	{
		if (!PopStack(Frame::Type::Compound))
		{
			return ResultControl::Stop;
		}
		return ResultControl::Continue;
	}

	/// @brief 整体扫描开始：初始化栈，将根节点设为当前帧
	/// @note 不会自动清除已有根节点数据，多次调用会合并结果。
	void VisitBegin(void)
	{
		vStack.clear();
		vStack.push_back(
			Frame
			{
				.enType = Frame::Type::Compound,
				.pCompound = &cpdRoot,
			}
		);
		sPendingKey.clear();
		//cpdRoot.Clear();//不清理，方便拼接，用户需要可自行清理
		return;
	}

	/// @brief 整体扫描结束：清理栈帧
	void VisitEnd(void)
	{
		vStack.pop_back();
		vStack.clear();
		vStack.shrink_to_fit();
		return;
	}

	/// @copydoc NBT_Visitor::VisitError
	template<typename... Args>
	void VisitError(NBT_Print_Level lvl, const std::format_string<Args...> fmt, Args&&... args) noexcept
	{
		NBT_Print{}(lvl, fmt, std::forward<Args>(args)...);
		return;
	}
};

static_assert(IsLookLike_NBT_Visitor<NBT_Visitor_Collector>);
