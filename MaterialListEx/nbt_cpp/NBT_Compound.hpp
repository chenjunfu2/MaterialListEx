#pragma once

#include <unordered_map>
#include <compare>
#include <type_traits>
#include <initializer_list>

#include "NBT_Type.hpp"

/// @file
/// @brief NBT集合基础类型

class NBT_Reader;
class NBT_Writer;
class NBT_Helper;

/// @brief 用于存放NBT_Compound使用的，无法存在于类内的概念
/// @note 用户不应直接使用此内容
namespace NBT_Compound_Concept
{
	/// @brief 概念约束，检查类型T是否支持三路比较运算符（<=>）
	template <typename T>
	concept HasSpaceship = requires(const T & a, const T & b)
	{
		{
			a <=> b
		};
	};

	/// @brief 概念约束，检查类型T是否具有rbegin()成员函数
	template <typename T>
	concept HasRBegin = requires(T t)
	{
		t.rbegin();
	};

	/// @brief 概念约束，检查类型T是否具有crbegin()成员函数
	template <typename T>
	concept HasCRBegin = requires(T t)
	{
		t.crbegin();
	};

	/// @brief 概念约束，检查类型T是否具有rend()成员函数
	template <typename T>
	concept HasREnd = requires(T t)
	{
		t.rend();
	};

	/// @brief 概念约束，检查类型T是否具有crend()成员函数
	template <typename T>
	concept HasCREnd = requires(T t)
	{
		t.crend();
	};
}

/// @brief 继承自标准库std::unordered_map的代理类，用于存储和管理NBT键值对
/// @tparam Compound 继承的父类，也就是std::unordered_map
/// @note 用户不应自行实例化此类，请使用NBT_Type::Compound来访问此类实例化类型
template<typename Compound>
class NBT_Compound :protected Compound//Compound is Map
{
	friend class NBT_Reader;
	friend class NBT_Writer;
	friend class NBT_Helper;

private:
	//总是允许插入nbt end，但是在写出文件时会忽略end类型
	//template<typename V>
	//bool TestType(V vTagVal)
	//{
	//	if constexpr (std::is_same_v<std::decay_t<V>, Compound::mapped_type>)
	//	{
	//		return vTagVal.GetTag() != NBT_TAG::End;
	//	}
	//	else
	//	{
	//		return NBT_Type::TypeTag_V<std::decay_t<V>> != NBT_TAG::End;
	//	}
	//}
public:
	//完美转发、初始化列表代理构造

	/// @brief 完美转发构造函数
	/// @tparam Args 构造参数类型包
	/// @param args 构造参数
	/// @note 将参数完美转发给底层容器进行构造
	template<typename... Args>
	NBT_Compound(Args&&... args) : Compound(std::forward<Args>(args)...)
	{}

	/// @brief 初始化列表构造函数
	/// @param init 初始化列表，包含要初始化的键值对
	/// @note 使用初始化列表直接构造底层容器
	NBT_Compound(std::initializer_list<typename Compound::value_type> init) : Compound(init)
	{}

	/// @brief 默认构造函数
	NBT_Compound(void) = default;
	/// @brief 默认析构函数
	~NBT_Compound(void) = default;

	/// @brief 移动构造函数
	/// @param _Move 要移动的源对象
	NBT_Compound(NBT_Compound &&_Move) noexcept :Compound(std::move(_Move))
	{}

	/// @brief 拷贝构造函数
	/// @param _Copy 要拷贝的源对象
	NBT_Compound(const NBT_Compound &_Copy) noexcept :Compound(_Copy)
	{}

	/// @brief 移动赋值运算符
	/// @param _Move 要移动的源对象
	/// @return 当前对象的引用
	NBT_Compound &operator=(NBT_Compound &&_Move) noexcept
	{
		Compound::operator=(std::move(_Move));
		return *this;
	}

	/// @brief 拷贝赋值运算符
	/// @param _Copy 要拷贝的源对象
	/// @return 当前对象的引用
	NBT_Compound &operator=(const NBT_Compound &_Copy)
	{
		Compound::operator=(_Copy);
		return *this;
	}

	/// @brief 获取底层容器数据的常量引用
	/// @return 底层容器数据的常量引用
	const Compound &GetData(void) const noexcept
	{
		return *this;
	}

	/// @brief 相等比较运算符
	/// @param _Right 要比较的右操作数
	/// @return 是否相等
	/// @note 转发底层容器的实现，具体信息请参考std::unordered_map的说明
	bool operator==(const NBT_Compound &_Right) const noexcept
	{
		return (const Compound &)*this == (const Compound &)_Right;
	}

	/// @brief 不等比较运算符
	/// @param _Right 要比较的右操作数
	/// @return 是否不相等
	/// @note 转发底层容器的实现，具体信息请参考std::unordered_map的说明
	bool operator!=(const NBT_Compound &_Right) const noexcept
	{
		return (const Compound &)*this != (const Compound &)_Right;
	}

	/// @brief 三路比较运算符
	/// @param _Right 要比较的右操作数
	/// @return 比较结果，通过std::partial_ordering返回
	/// @note 如果底层容器支持三路比较则转发其实现，否则使用容器的相等运算符比较，在相等时返回equivalent，不相等时返回unordered
	std::partial_ordering operator<=>(const NBT_Compound &_Right) const noexcept
	{
		if constexpr (NBT_Compound_Concept::HasSpaceship<Compound>)
		{
			return (const Compound &)*this <=> (const Compound &)_Right;
		}
		else
		{
			if (operator==(_Right))
			{
				return std::partial_ordering::equivalent;//相等返回相等
			}
			else
			{
				return std::partial_ordering::unordered;//否则返回不可比较
			}
		}
	}

	/// @name 暴露父类迭代器接口
	/// @brief 继承底层容器的迭代器和访问接口
	/// @note 部分api如果父类不存在，则自动隐藏
	/// @{

	using Compound::begin;
	using Compound::end;
	using Compound::cbegin;
	using Compound::cend;
	using Compound::operator[];

	//因为父类不总是有下面内容，所以使用requires检查并暴露或舍弃
	//using Compound::rbegin;
	//using Compound::rend;
	//using Compound::crbegin;
	//using Compound::crend;
	
	//存在则映射
	/// @cond
	//-------------------- rbegin --------------------
	auto rbegin() requires NBT_Compound_Concept::HasRBegin<Compound> {return Compound::rbegin();}
	auto rbegin() const requires NBT_Compound_Concept::HasRBegin<Compound> {return Compound::rbegin();}
	auto crbegin() const noexcept requires NBT_Compound_Concept::HasCRBegin<Compound> {return Compound::crbegin();}
	//-------------------- rend --------------------
	auto rend() requires NBT_Compound_Concept::HasREnd<Compound> {return Compound::rend();}
	auto rend() const requires NBT_Compound_Concept::HasREnd<Compound> {return Compound::rend();}
	auto crend() const noexcept requires NBT_Compound_Concept::HasCREnd<Compound> {return Compound::crend();}
	/// @endcond

	/// @}

	//简化map查询

	/// @brief 根据标签名获取对应的NBT值
	/// @param sTagName 要查找的标签名
	/// @return 标签名对应的值的引用
	/// @note 如果标签不存在则抛出异常，具体请参考std::unordered_map关于at的说明
	typename Compound::mapped_type &Get(const typename Compound::key_type &sTagName)
	{
		return Compound::at(sTagName);
	}

	/// @brief 根据标签名获取对应的NBT值（常量版本）
	/// @param sTagName 要查找的标签名
	/// @return 标签名对应的值的常量引用
	/// @note 如果标签不存在则抛出异常，具体请参考std::unordered_map关于at的说明
	const typename Compound::mapped_type &Get(const typename Compound::key_type &sTagName) const
	{
		return Compound::at(sTagName);
	}


	/// @brief 搜索标签是否存在
	/// @param sTagName 要搜索的标签名
	/// @return 如果找到，则返回指向标签名对应的值的指针，否则返回NULL指针
	/// @note 标签不存在时不会抛出异常，适用于检查性访问
	typename Compound::mapped_type *Has(const typename Compound::key_type &sTagName) noexcept
	{
		auto find = Compound::find(sTagName);
		return find == Compound::end()
			? NULL
			: &(find->second);
	}

	/// @brief 搜索标签是否存在（常量版本）
	/// @param sTagName 要搜索的标签名
	/// @return 如果找到，则返回指向标签名对应的值的常量指针，否则返回NULL指针
	/// @note 标签不存在时不会抛出异常，适用于检查性访问
	const typename Compound::mapped_type *Has(const typename Compound::key_type &sTagName) const noexcept
	{
		auto find = Compound::find(sTagName);
		return find == Compound::end()
			? NULL
			: &(find->second);
	}

	//简化map插入
	//使用完美转发，不丢失引用、右值信息

	/// @brief 插入或替换键值对
	/// @tparam K 标签名类型，必须可构造为key_type
	/// @tparam V 标签值类型，必须可构造为mapped_type
	/// @param sTagName 标签名
	/// @param vTagVal 标签名对应的值
	/// @return 包含迭代器和bool值的pair，迭代器指向插入或替换的元素，bool值为true则执行了插入，否则执行了替换
	/// @note 允许插入End类型的值，这样设计的目的在于允许处理过程出现特殊值，
	/// 请确保输出前处理完毕所有End类型，否则通过NBT_Write输出时会忽略这些值并生成警告
	template <typename K, typename V>
	requires std::constructible_from<typename Compound::key_type, K &&> &&std::constructible_from<typename Compound::mapped_type, V &&>
	std::pair<typename Compound::iterator, bool> Put(K &&sTagName, V &&vTagVal)
	{
		//总是允许插入nbt end，但是在写出文件时会忽略end类型
		//if (!TestType(vTagVal))
		//{
		//	return std::pair{ Compound::end(),false };
		//}

		return Compound::insert_or_assign(std::forward<K>(sTagName), std::forward<V>(vTagVal));
	}

	/// @brief 原位构造键值对
	/// @tparam K 标签名类型，必须可构造为key_type
	/// @tparam V 标签值类型，必须可构造为mapped_type
	/// @param sTagName 标签名
	/// @param vTagVal 标签名对应的值
	/// @return 包含迭代器和bool值的pair，迭代器指向插入或替换的元素，bool值为true则执行了插入，否则键已存在，不会进行拷贝或移动
	/// @note 允许插入End类型的值，这样设计的目的在于允许处理过程出现特殊值，
	/// 请确保输出前处理完毕所有End类型，否则通过NBT_Write输出时会忽略这些值并生成警告
	template <typename K, typename V>
	requires std::constructible_from<typename Compound::key_type, K &&> &&std::constructible_from<typename Compound::mapped_type, V &&>
	std::pair<typename Compound::iterator, bool> TryPut(K &&sTagName, V &&vTagVal)
	{
		//总是允许插入nbt end，但是在写出文件时会忽略end类型
		//if (!TestType(vTagVal))
		//{
		//	return std::pair{ Compound::end(),false };
		//}

		return Compound::try_emplace(std::forward<K>(sTagName), std::forward<V>(vTagVal));
	}

	/// @brief 删除指定标签
	/// @param sTagName 要删除的标签名
	/// @return 是否成功删除（标签存在且被删除返回true，否则返回false）
	bool Remove(const typename Compound::key_type &sTagName)
	{
		return Compound::erase(sTagName) != 0;//返回1即为成功，否则为0，标准库：返回值为删除的元素数（0 或 1）。
	}

	/// @brief 清空所有标签
	/// @note 移除容器中的所有键值对，容器大小变为0
	void Clear(void)
	{
		Compound::clear();
	}

	/// @brief 检查容器是否为空
	/// @return 如果容器为空返回true，否则返回false
	bool Empty(void) const noexcept
	{
		return Compound::empty();
	}

	/// @brief 获取容器中元素的数量
	/// @return 容器中键值对的数量
	size_t Size(void) const noexcept
	{
		return Compound::size();
	}

	/// @brief 合并另一个NBT_Compound的内容（拷贝）
	/// @param _Copy 要合并的源对象
	/// @note 将源对象的所有键值对合并到当前对象，重复的键不会被覆盖，
	/// 具体行为请参考std::unordered_map对于merge的说明
	void Merge(const NBT_Compound &_Copy)
	{
		Compound::merge(_Copy);
	}

	/// @brief 合并另一个NBT_Compound的内容（移动）
	/// @param _Move 要合并的源对象
	/// @note 将源对象的所有键值对合并到当前对象，重复的键不会被覆盖，
	/// 具体行为请参考std::unordered_map关于merge的说明
	void Merge(NBT_Compound &&_Move)
	{
		Compound::merge(std::move(_Move));
	}

	/// @brief 检查是否包含指定标签
	/// @param sTagName 要检查的标签名
	/// @return 如果包含指定标签返回true，否则返回false
	bool Contains(const typename Compound::key_type &sTagName) const noexcept
	{
		return Compound::contains(sTagName);
	}

	/// @brief 使用谓词检查是否存在满足条件的元素
	/// @tparam Predicate 谓词仿函数类型，需要接受value_type并返回bool
	/// @param pred 谓词仿函数对象
	/// @return 如果存在满足条件的元素返回true，否则返回false
	/// @note 具体用法参见标准库针对std::find_if的说明
	template<typename Predicate>
	bool ContainsIf(Predicate pred) const noexcept
	{
		return std::find_if(Compound::begin(), Compound::end(), pred) != Compound::end();
	}


//针对每种类型生成一个方便的函数
//通过宏定义批量生成

/// @def TYPE_GET_FUNC(type)
/// @brief 不同类型名接口生成宏
/// @note 用户不应该使用此宏（实际上宏已在使用后取消定义），标注仅为消除doxygen警告
#define TYPE_GET_FUNC(type)\
/**
 @brief 检查是否包含指定标签名的 type 类型数据
 @param sTagName 要检查的标签名
 @return 如果包含指定标签名，且对应的值的类型匹配，则返回true，否则返回false
 @note 同时检查标签存在性和值类型
 */\
bool Contains##type(const typename Compound::key_type &sTagName) const\
{\
	auto *p = Has(sTagName);\
	return p != NULL && p->GetTag() == NBT_TAG::type;\
}\
\
/**
 @brief 获取指定标签名的 type 类型数据（常量版本）
 @param sTagName 标签名
 @return type 类型数据的常量引用
 @note 如果标签不存在或类型不匹配则抛出异常，
 具体请参考std::unordered_map关于at的说明与std::get的说明
 */\
const typename NBT_Type::type &Get##type(const typename Compound::key_type & sTagName) const\
{\
	return Compound::at(sTagName).Get##type();\
}\
\
/**
 @brief 获取指定标签名的 type 类型数据
 @param sTagName 标签名
 @return type 类型数据的引用
 @note 如果标签不存在或类型不匹配则抛出异常，
 具体请参考std::unordered_map关于at的说明与std::get的说明
 */\
typename NBT_Type::type &Get##type(const typename Compound::key_type & sTagName)\
{\
	return Compound::at(sTagName).Get##type();\
}\
\
/**
 @brief 安全检查并获取指定标签名的 type 类型数据（常量版本）
 @param sTagName 标签名
 @return 如果存在且对应值的类型为 type 则返回指向数据的常量指针，否则返回NULL
 @note 标签不存在或类型不为 type 时不会抛出异常，适用于检查性访问
 */\
const typename NBT_Type::type *Has##type(const typename Compound::key_type & sTagName) const noexcept\
{\
	auto *p = Has(sTagName);\
	return p != NULL && p->Is##type()\
		? &(p->Get##type())\
		: NULL;\
}\
\
/**
 @brief 安全检查并获取指定标签名的 type 类型数据
 @param sTagName 标签名
 @return 如果存在且对应值的类型为 type 则返回指向数据的指针，否则返回NULL
 @note 标签不存在或类型不为 type 时不会抛出异常，适用于检查性访问
 */\
typename NBT_Type::type *Has##type(const typename Compound::key_type & sTagName) noexcept\
{\
	auto *p = Has(sTagName);\
	return p != NULL && p->Is##type()\
		? &(p->Get##type())\
		: NULL;\
}

	/// @name 针对每种类型提供一个方便使用的函数，由宏批量生成
	/// @brief 具体作用说明：
	/// - Get开头+类型名的函数：直接获取指定标签名且对应类型的引用，异常由std::unordered_map的at与std::get具体实现决定
	/// - Has开头 + 类型名的函数：判断指定标签名是否存在，且标签名对应的类型是否是指定类型，都符合则返回对应指针，否则返回NULL指针
	/// @{

	TYPE_GET_FUNC(End);
	TYPE_GET_FUNC(Byte);
	TYPE_GET_FUNC(Short);
	TYPE_GET_FUNC(Int);
	TYPE_GET_FUNC(Long);
	TYPE_GET_FUNC(Float);
	TYPE_GET_FUNC(Double);
	TYPE_GET_FUNC(ByteArray);
	TYPE_GET_FUNC(IntArray);
	TYPE_GET_FUNC(LongArray);
	TYPE_GET_FUNC(String);
	TYPE_GET_FUNC(List);
	TYPE_GET_FUNC(Compound);

	/// @}

#undef TYPE_GET_FUNC

/// @def TYPE_PUT_FUNC(type)
/// @brief 不同类型名接口生成宏
/// @note 用户不应该使用此宏（实际上宏已在使用后取消定义），标注仅为消除doxygen警告
#define TYPE_PUT_FUNC(type)\
/**
 @brief 插入或替换 type 类型的键值对（拷贝）
 @tparam K 标签名类型，必须可构造为key_type
 @param sTagName 标签名
 @param vTagVal 要插入的 type 类型值
 @return 包含迭代器和bool值的pair，迭代器指向插入或替换的元素，bool值为true则执行了插入，否则执行了替换
 @note 允许插入End类型的值，这样设计的目的在于允许处理过程出现特殊值，
 请确保输出前处理完毕所有End类型，否则通过NBT_Write输出时会忽略这些值并生成警告
 */\
template <typename K>\
requires std::constructible_from<typename Compound::key_type, K &&>\
std::pair<typename Compound::iterator, bool> Put##type(K &&sTagName, const typename NBT_Type::type &vTagVal)\
{\
	return Put(std::forward<K>(sTagName), vTagVal);\
}\
\
/**
 @brief 插入或替换 type 类型的键值对（移动）
 @tparam K 标签名类型，必须可构造为key_type
 @param sTagName 标签名
 @param vTagVal 要插入的 type 类型值
 @return 包含迭代器和bool值的pair，迭代器指向插入或替换的元素，bool值为true则执行了插入，否则执行了替换
 @note 允许插入End类型的值，这样设计的目的在于允许处理过程出现特殊值，
 请确保输出前处理完毕所有End类型，否则通过NBT_Write输出时会忽略这些值并生成警告
 */\
template <typename K>\
requires std::constructible_from<typename Compound::key_type, K &&>\
std::pair<typename Compound::iterator, bool> Put##type(K &&sTagName, typename NBT_Type::type &&vTagVal)\
{\
	return Put(std::forward<K>(sTagName), std::move(vTagVal));\
}\
\
 /**
 @brief 尝试插入 type 类型的键值对（拷贝）
 @tparam K 标签名类型，必须可构造为key_type
 @param sTagName 标签名
 @param vTagVal 要插入的 type 类型值
 @return 包含迭代器和bool值的pair，迭代器指向插入或替换的元素，bool值为true则执行了插入，否则键已存在，不会进行拷贝或移动
 @note 允许插入End类型的值，这样设计的目的在于允许处理过程出现特殊值，
 请确保输出前处理完毕所有End类型，否则通过NBT_Write输出时会忽略这些值并生成警告
 */\
template <typename K>\
requires std::constructible_from<typename Compound::key_type, K &&>\
std::pair<typename Compound::iterator, bool> TryPut##type(K &&sTagName, const typename NBT_Type::type &vTagVal)\
{\
	return TryPut(std::forward<K>(sTagName), vTagVal);\
}\
\
/**
 @brief 尝试插入 type 类型的键值对（移动）
 @tparam K 标签名类型，必须可构造为key_type
 @param sTagName 标签名
 @param vTagVal 要插入的 type 类型值
 @return 包含迭代器和bool值的pair，迭代器指向插入或替换的元素，bool值为true则执行了插入，否则键已存在，不会进行拷贝或移动
 @note 允许插入End类型的值，这样设计的目的在于允许处理过程出现特殊值，
 请确保输出前处理完毕所有End类型，否则通过NBT_Write输出时会忽略这些值并生成警告
 */\
template <typename K>\
requires std::constructible_from<typename Compound::key_type, K &&>\
std::pair<typename Compound::iterator, bool> TryPut##type(K &&sTagName, typename NBT_Type::type &&vTagVal)\
{\
	return TryPut(std::forward<K>(sTagName), std::move(vTagVal));\
}

	/// @name 针对每种类型提供插入函数，由宏批量生成
	/// @brief 具体作用说明：
	/// - Put开头+类型名的函数：插入指定类型的数据到指定标签名
	/// @{

	TYPE_PUT_FUNC(End);
	TYPE_PUT_FUNC(Byte);
	TYPE_PUT_FUNC(Short);
	TYPE_PUT_FUNC(Int);
	TYPE_PUT_FUNC(Long);
	TYPE_PUT_FUNC(Float);
	TYPE_PUT_FUNC(Double);
	TYPE_PUT_FUNC(ByteArray);
	TYPE_PUT_FUNC(IntArray);
	TYPE_PUT_FUNC(LongArray);
	TYPE_PUT_FUNC(String);
	TYPE_PUT_FUNC(List);
	TYPE_PUT_FUNC(Compound);

	/// @}

#undef TYPE_PUT_FUNC
};
