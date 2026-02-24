#pragma once

#include <vector>
#include <compare>
#include <type_traits>
#include <initializer_list>
#include <stdexcept>

#include "NBT_Type.hpp"

/// @file
/// @brief NBT列表类型

class NBT_Reader;
class NBT_Writer;
class NBT_Helper;

/// @brief 继承自标准库容器的代理类，用于存储和管理NBT列表
/// @tparam List 继承的父类，也就是std::vector
/// @note 用户不应自行实例化此类，请使用NBT_Type::List来访问此类实例化类型
template <typename List>
class NBT_List :protected List
{
	friend class NBT_Reader;
	friend class NBT_Writer;
	friend class NBT_Helper;
	
public:
	//完美转发、初始化列表代理构造

	/// @brief 构造函数
	/// @tparam Args 变长构造参数类型包
	/// @param args 变长构造参数列表
	template<typename... Args>
	NBT_List(Args&&... args) : List(std::forward<Args>(args)...)
	{}

	/// @brief 初始化列表构造函数
	/// @param init 初始化列表
	NBT_List(std::initializer_list<typename List::value_type> init) : List(init)
	{}

	/// @brief 默认构造函数
	NBT_List(void) = default;
	/// @brief 析构函数
	~NBT_List(void) = default;

	/// @brief 移动构造函数
	/// @param _Move 要移动的源对象
	NBT_List(NBT_List &&_Move) noexcept :List(std::move(_Move))
	{}

	/// @brief 拷贝构造函数
	/// @param _Copy 要拷贝的源对象
	NBT_List(const NBT_List &_Copy) :List(_Copy)
	{}

	/// @brief 移动赋值运算符
	/// @param _Move 要移动的源对象
	/// @return 当前对象的引用
	NBT_List &operator=(NBT_List &&_Move) noexcept
	{
		List::operator=(std::move(_Move));
		return *this;
	}

	/// @brief 拷贝赋值运算符
	/// @param _Copy 要拷贝的源对象
	/// @return 当前对象的引用
	NBT_List &operator=(const NBT_List &_Copy)
	{
		List::operator=(_Copy);
		return *this;
	}

	/// @brief 获取底层容器数据的常量引用
	/// @return 底层容器数据的常量引用
	const List &GetData(void) const noexcept
	{
		return *this;
	}

	/// @brief 相等比较运算符
	/// @param _Right 要比较的右操作数
	/// @return 是否相等
	bool operator==(const NBT_List &_Right) const noexcept
	{
		return (const List &)*this == (const List &)_Right;
	}

	/// @brief 不等比较运算符
	/// @param _Right 要比较的右操作数
	/// @return 是否不相等
	bool operator!=(const NBT_List &_Right) const noexcept
	{
		return (const List &)*this != (const List &)_Right;
	}

	/// @brief 三路比较运算符
	/// @param _Right 要比较的右操作数
	/// @return 比较结果，通过std::partial_ordering返回
	std::partial_ordering operator<=>(const NBT_List &_Right) const noexcept
	{
		return (const List &)*this <=> (const List &)_Right;
	}

	/// @name 暴露父类迭代器接口
	/// @brief 继承底层容器的迭代器和访问接口
	/// @{

	using List::begin;
	using List::end;
	using List::cbegin;
	using List::cend;
	using List::rbegin;
	using List::rend;
	using List::crbegin;
	using List::crend;
	using List::operator[];

	/// @}

	/// @name 查询接口
	/// @brief 提供一组接口用于对list不同元素的访问
	/// @{

	/// @brief 根据位置获取值
	/// @param szPos 要查找的位置
	/// @return 位置对应的值的引用
	/// @note 如果位置不存在则抛出异常，请参考std::vector对于at的描述
	typename List::value_type &Get(const typename List::size_type &szPos)
	{
		return List::at(szPos);
	}

	/// @brief 根据位置获取值（常量版本）
	/// @param szPos 要查找的位置
	/// @return 位置对应的值的常量引用
	/// @note 如果位置不存在则抛出异常，请参考std::vector对于at的描述
	const typename List::value_type &Get(const typename List::size_type &szPos) const
	{
		return List::at(szPos);
	}

	/// @brief 根据位置查找值
	/// @param szPos 要查找的位置
	/// @return 位置对应的值的指针，如果值不存在则为NULL
	typename List::value_type *Has(const typename List::size_type &szPos) noexcept
	{
		return szPos < List::size()
			? &List::operator[](szPos)
			: NULL;
	}

	/// @brief 根据位置查找值（常量版本）
	/// @param szPos 要查找的位置
	/// @return 位置对应的值的指针，如果值不存在则为NULL
	const typename List::value_type *Has(const typename List::size_type &szPos) const noexcept
	{
		return szPos < List::size()
			? &List::operator[](szPos)
			: NULL;
	}

	/// @brief 获取列表开头的元素
	/// @return 开头的元素的引用
	/// @note 如果当前列表为空，行为未定义，请参考std::vector对于front的描述
	typename List::value_type &Front(void) noexcept
	{
		return List::front();
	}

	/// @brief 获取列表开头的元素（常量版本）
	/// @return 开头的元素的常量引用
	/// @note 如果当前列表为空，行为未定义，请参考std::vector对于front的描述
	const typename List::value_type &Front(void) const noexcept
	{
		return List::front();
	}

	/// @brief 获取列表最后的元素
	/// @return 最后的元素的引用
	/// @note 如果当前列表为空，行为未定义，请参考std::vector对于back的描述
	typename List::value_type &Back(void) noexcept
	{
		return List::back();
	}

	/// @brief 获取列表最后的元素（常量版本）
	/// @return 最后的元素的引用
	/// @note 如果当前列表为空，行为未定义，请参考std::vector对于back的描述
	const typename List::value_type &Back(void) const noexcept
	{
		return List::back();
	}

	/// @}

	/// @name 修改接口
	/// @brief 提供一组接口用于对list进行元素编辑
	/// @todo 提供范围插入等功能
	/// @{
	
	/// @brief 在指定位置的前面插入元素
	/// @tparam V 元素值类型
	/// @param szPos 插入位置
	/// @param vTagVal 要插入的值
	/// @return 刚才添加的元素的引用
	/// @note 用户需要保证szPos范围合法，否则行为未定义
	template <typename V>
	typename List::value_type &Add(size_t szPos, V &&vTagVal)
	{
		return *List::emplace(List::begin() + szPos, std::forward<V>(vTagVal));//插入
	}

	/// @brief 在列表头部插入元素
	/// @tparam V 元素值类型
	/// @param vTagVal 要插入的值
	/// @return 刚才添加的元素的引用
	template <typename V>
	typename List::value_type &AddFront(V &&vTagVal)
	{
		return *List::emplace(List::begin(), std::forward<V>(vTagVal));//插入
	}

	/// @brief 在列表末尾插入元素
	/// @tparam V 元素值类型
	/// @param vTagVal 要插入的值
	/// @return 刚才添加的元素的引用
	template <typename V>
	typename List::value_type &AddBack(V &&vTagVal)
	{
		return List::emplace_back(std::forward<V>(vTagVal));
	}

	/// @brief 设置（替换）指定位置的元素
	/// @tparam V 元素值类型
	/// @param szPos 要设置的位置
	/// @param vTagVal 要设置的值
	/// @return 刚才设置的元素的引用
	/// @note 用户需要保证szPos范围合法，否则行为未定义
	template <typename V>
	typename List::value_type &Set(size_t szPos, V &&vTagVal)
	{
		return List::operator[](szPos) = std::forward<V>(vTagVal);
	}

	/// @brief 删除指定位置的元素
	/// @param szPos 要删除的位置
	void Remove(size_t szPos)
	{
		List::erase(List::begin() + szPos);//这个没必要返回结果，直接丢弃
	}

	/// @brief 清空所有元素
	/// @note 元素清空后，列表允许直接插入任意类型的元素
	void Clear(void)
	{
		List::clear();
	}

	/// @brief 调整容器大小，如果大小大于当前大小，那么使用默认值填充新增空间，否则删除多余元素
	/// @param szNewSize 新的容器大小
	void Resize(size_t szNewSize)
	{
		return List::resize(szNewSize);
	}

	/// @brief 调整容器大小，如果大小大于当前大小，那么使用val填充新增空间，否则删除多余元素
	/// @param szNewSize 新的容器大小
	/// @param val （可能）需要重复的元素
	void Resize(size_t szNewSize, const typename List::value_type &val)
	{
		return List::resize(szNewSize, val);
	}

	/// @brief 拷贝合并另一个NBT_List的内容
	/// @param _Copy 要合并的源对象
	void Merge(const NBT_List &_Copy)
	{
		List::insert(List::end(), _Copy.begin(), _Copy.end());
	}

	/// @brief 移动合并另一个NBT_List的内容
	/// @param _Move 要合并的源对象
	void Merge(NBT_List &&_Move)
	{
		List::insert(List::end(), std::make_move_iterator(_Move.begin()), std::make_move_iterator(_Move.end()));
	}

	///@}

	/// @brief 检查容器是否为空
	/// @return 如果容器为空返回true，否则返回false
	bool Empty(void) const noexcept
	{
		return List::empty();
	}

	/// @brief 获取容器中元素的数量
	/// @return 容器中元素的数量
	size_t Size(void) const noexcept
	{
		return List::size();
	}

	/// @brief 预留存储空间
	/// @param szNewCap 新的容量大小
	void Reserve(size_t szNewCap)
	{
		return List::reserve(szNewCap);
	}

	/// @brief 缩减容器容量以匹配大小
	void ShrinkToFit(void)
	{
		return List::shrink_to_fit();
	}

	/// @brief 检查是否包含指定元素
	/// @param tValue 要检查的元素
	/// @return 如果包含指定元素返回true，否则返回false
	/// @note 与NBT_Compound进行哈希查找不同，这里是通过遍历实现的，请注意开销
	bool Contains(const typename List::value_type &tValue) const noexcept
	{
		return std::find(List::begin(), List::end(), tValue) != List::end();
	}

	/// @brief 使用谓词检查是否存在满足条件的元素
	/// @tparam Predicate 谓词仿函数类型，需要接受value_type并返回bool
	/// @param pred 谓词仿函数对象
	/// @return 如果存在满足条件的元素返回true，否则返回false
	/// @note 与NBT_Compound进行哈希查找不同，这里是通过遍历实现的，请注意开销
	template<typename Predicate>
	bool ContainsIf(Predicate pred) const noexcept
	{
		return std::find_if(List::begin(), List::end(), pred) != List::end();
	}

/// @def TYPE_GET_FUNC(type)
/// @brief 不同类型名接口生成宏
/// @note 用户不应该使用此宏（实际上宏已在使用后取消定义），标注仅为消除doxygen警告
#define TYPE_GET_FUNC(type)\
/**
 @brief 获取指定位置的 type 类型数据（常量版本）
 @param szPos 位置索引
 @return type 类型数据的常量引用
 @note 如果位置不存在或类型不匹配则抛出异常，
 具体请参考std::vector关于at的说明与std::get的说明
 */\
const typename NBT_Type::type &Get##type(const typename List::size_type &szPos) const\
{\
	return List::at(szPos).Get##type();\
}\
\
/**
 @brief 获取指定位置的 type 类型数据
 @param szPos 位置索引
 @return type 类型数据的引用
 @note 如果位置不存在或类型不匹配则抛出异常，
 具体请参考std::vector关于at的说明与std::get的说明
 */\
typename NBT_Type::type &Get##type(const typename List::size_type &szPos)\
{\
	return List::at(szPos).Get##type();\
}\
\
/**
 @brief 获取指定位置的 type 类型数据（常量版本）
 @param szPos 位置索引
 @return type 类型数据的指针，如果位置不存在或类型不对则返回NULL
 */\
const typename NBT_Type::type *Has##type(const typename List::size_type &szPos) const noexcept\
{\
	auto *p = Has(szPos);\
	return p != NULL && p->Is##type()\
		? &p->Get##type()\
		: NULL;\
}\
\
/**
 @brief 获取指定位置的 type 类型数据
 @param szPos 位置索引
 @return type 类型数据的指针，如果位置不存在或类型不对则返回NULL
 */\
typename NBT_Type::type *Has##type(const typename List::size_type &szPos) noexcept\
{\
	auto *p = Has(szPos);\
	return p != NULL && p->Is##type()\
		? &p->Get##type()\
		: NULL;\
}\
\
/**
 @brief 获取列表第一个 type 类型数据（常量版本）
 @return type 类型数据的常量引用
 @note 如果列表为空或类型不匹配则抛出异常，
 具体请参考std::vector关于front的说明与std::get的说明
 */\
const typename NBT_Type::type &Front##type(void) const\
{\
	return List::front().Get##type(); \
}\
\
/**
 @brief 获取列表第一个 type 类型数据
 @return type 类型数据的引用
 @note 如果列表为空或类型不匹配则抛出异常，
 具体请参考std::vector关于front的说明与std::get的说明
 */\
typename NBT_Type::type &Front##type(void)\
 {\
	return List::front().Get##type(); \
 }\
/**
 @brief 获取列表最后一个 type 类型数据（常量版本）
 @return type 类型数据的常量引用
 @note 如果列表为空或类型不匹配则抛出异常，
 具体请参考std::vector关于back的说明与std::get的说明
 */\
const typename NBT_Type::type &Back##type(void) const\
{\
	return List::back().Get##type();\
}\
\
/**
 @brief 获取列表最后一个 type 类型数据
 @return type 类型数据的引用
 @note 如果列表为空或类型不匹配则抛出异常，
 具体请参考std::vector关于back的说明与std::get的说明
 */\
typename NBT_Type::type &Back##type(void)\
{\
	return List::back().Get##type();\
}

 /// @name 针对每种类型提供一个方便使用的函数，由宏批量生成
 /// @brief 具体作用说明：
 /// - Get开头+类型名的函数：直接获取指定位置且对应类型的引用，异常由std::vector的at与std::get具体实现决定
 /// - Front开头+类型名的函数：获取列表第一个对应类型的元素引用
 /// - Back开头+类型名的函数：获取列表最后一个对应类型的元素引用
 /// @note 请不要使用这些API修改list内部对象的类型（注意是类型而非值），
 /// 这些接口无法简单的进行封装并检查用户对类型的操作
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
 @brief 在指定位置插入 type 类型数据（拷贝）
 @param szPos 插入位置
 @param vTagVal 要插入的 type 类型值
 @return 刚才插入的元素的引用
 */\
typename List::value_type &Add##type(size_t szPos, const typename NBT_Type::type &vTagVal)\
{\
	return Add(szPos, vTagVal);\
}\
\
/**
 @brief 在指定位置插入 type 类型数据（移动）
 @param szPos 插入位置
 @param vTagVal 要插入的 type 类型值
 @return 刚才插入的元素的引用
 @note 通用类型函数Add的代理，具体行为参考Add函数的说明
 */\
typename List::value_type & Add##type(size_t szPos, typename NBT_Type::type &&vTagVal)\
{\
	return Add(szPos, std::move(vTagVal));\
}\
\
/**
 @brief 在列表头部插入 type 类型数据（拷贝）
 @param vTagVal 要插入的 type 类型值
 @return 刚才插入的元素的引用
 @note 通用类型函数AddFront的代理，具体行为参考AddFront函数的说明
 */\
typename List::value_type &AddFront##type(const typename NBT_Type::type &vTagVal)\
{\
	return AddFront(vTagVal); \
}\
\
/**
 @brief 在列表头部插入 type 类型数据（移动）
 @param vTagVal 要插入的 type 类型值
 @return 刚才插入的元素的引用
 @note 通用类型函数AddFront的代理，具体行为参考AddFront函数的说明
 */\
typename List::value_type & AddFront##type(typename NBT_Type::type &&vTagVal)\
{\
	return AddFront(std::move(vTagVal));\
}\
\
/**
 @brief 在列表末尾插入 type 类型数据（拷贝）
 @param vTagVal 要插入的 type 类型值
 @return 刚才插入的元素的引用
 @note 通用类型函数AddBack的代理，具体行为参考AddBack函数的说明
 */\
typename List::value_type &AddBack##type(const typename NBT_Type::type &vTagVal)\
{\
	return AddBack(vTagVal);\
}\
\
/**
 @brief 在列表末尾插入 type 类型数据（移动）
 @param vTagVal 要插入的 type 类型值
 @return 刚才插入的元素的引用
 @note 通用类型函数AddBack的代理，具体行为参考AddBack函数的说明
 */\
typename List::value_type &AddBack##type(typename NBT_Type::type &&vTagVal)\
{\
	return AddBack(std::move(vTagVal));\
}\
\
/**
 @brief 设置指定位置的 type 类型数据（拷贝）
 @param szPos 要设置的位置
 @param vTagVal 要设置的 type 类型值
 @return 刚才修改的元素的引用
 @note 通用类型函数Set的代理，具体行为参考Set函数的说明
 */\
typename List::value_type &Set##type(size_t szPos, const typename NBT_Type::type &vTagVal)\
{\
	return Set(szPos, vTagVal);\
}\
\
/**
 @brief 设置指定位置的 type 类型数据（移动）
 @param szPos 要设置的位置
 @param vTagVal 要设置的 type 类型值
 @return 刚才修改的元素的引用
 @note 通用类型函数Set的代理，具体行为参考Set函数的说明
 */\
typename List::value_type &Set##type(size_t szPos, typename NBT_Type::type &&vTagVal)\
{\
	return Set(szPos, std::move(vTagVal));\
}

	/// @name 针对每种类型提供插入和设置函数，由宏批量生成
	/// @brief 具体作用说明：
	/// - Add开头+类型名的函数：在指定位置插入指定类型的数据
	/// - AddFront开头+类型名的函数：在列表末尾插入指定类型的数据
	/// - AddBack开头+类型名的函数：在列表末尾插入指定类型的数据
	/// - Set开头+类型名的函数：设置指定位置的指定类型数据
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