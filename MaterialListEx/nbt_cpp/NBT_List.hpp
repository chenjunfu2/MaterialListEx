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

private:
	//列表元素类型（同一时间list内只能存储一种类型的多个元素）
	//此变量仅用于规约列表元素类型，无需参与比较与hash，但是需要参与构造与移动
	NBT_TAG enElementTag = NBT_TAG::End;
	
private:
	bool TestTagAndSetType(NBT_TAG enTargetTag)
	{
		if (enTargetTag == NBT_TAG::End)
		{
			return false;//不能插入空值，拒掉
		}

		if (enElementTag == NBT_TAG::End)
		{
			enElementTag = enTargetTag;//当前为空状态，替换
			return true;
		}

		return enTargetTag == enElementTag;
	}

	template<typename V>
	bool TestAndSetType(const V &vTagVal)
	{
		if constexpr (std::is_same_v<std::decay_t<V>, typename List::value_type>)
		{
			return TestTagAndSetType(vTagVal.GetTag());
		}
		else
		{
			return TestTagAndSetType(NBT_Type::TypeTag_V<std::decay_t<V>>);
		}
	}

	void TestInit(void)
	{
		if (enElementTag == NBT_TAG::End && !List::empty())
		{
			Clear();
			throw std::invalid_argument("Tag is end but the list is not empty");
		}

		if (enElementTag != NBT_TAG::End)
		{
			for (const auto &it : (List)*this)
			{
				if (it.GetTag() != enElementTag)
				{
					Clear();
					throw std::invalid_argument("List contains elements of different types");
				}
			}
		}
	}
public:
	/// @brief 提示性标签，用于取消构造函数对类型符合性的检测，类型检查要求列表中所有元素类型一致
	/// @note 仅用于性能敏感场景，用户需保证类型一致性，否则在NBT_Write进行写出时将产生错误
	struct NoCheck_T
	{
		explicit constexpr NoCheck_T() noexcept = default;
	};

	/// @copydoc NoCheck_T
	constexpr static inline const NoCheck_T NoCheck{};

	//完美转发、初始化列表代理构造

	/// @brief 构造函数
	/// @tparam Args 变长构造参数类型包
	/// @param _enElementTag 列表元素类型标签
	/// @param args 变长构造参数列表
	/// @note 列表元素类型设置为_enElementTag代表的类型，并进行类型检查，
	/// 如果构造的类型不匹配，则抛出std::invalid_argument异常
	template<typename... Args>
	NBT_List(NBT_TAG _enElementTag, Args&&... args) :List(std::forward<Args>(args)...), enElementTag(_enElementTag)
	{
		TestInit();
	}

	/// @brief 构造函数
	/// @tparam Args 变长构造参数类型包
	/// @param _NoCheck 取消检查标记
	/// @param _enElementTag 列表元素类型标签
	/// @param args 变长构造参数列表
	/// @note 不进行类型检查
	template<typename... Args>
	NBT_List(NoCheck_T _NoCheck, NBT_TAG _enElementTag, Args&&... args) : List(std::forward<Args>(args)...), enElementTag(_enElementTag)
	{}

	/// @brief 构造函数
	/// @tparam Args 变长构造参数类型包
	/// @param args 变长构造参数列表
	/// @note 根据第一个元素的类型作为列表元素类型，并进行类型检查，
	/// 如果构造的类型不匹配，则抛出std::invalid_argument异常
	template<typename... Args>
	NBT_List(Args&&... args) : List(std::forward<Args>(args)...), enElementTag(List::empty() ? NBT_TAG::End : List::front().GetTag())
	{
		TestInit();
	}

	/// @brief 构造函数
	/// @tparam Args 变长构造参数类型包
	/// @param _NoCheck 取消检查标记
	/// @param args 变长构造参数列表
	/// @note 不进行类型检查
	template<typename... Args>
	NBT_List(NoCheck_T _NoCheck, Args&&... args) : List(std::forward<Args>(args)...), enElementTag(List::empty() ? NBT_TAG::End : List::front().GetTag())
	{}

	/// @brief 初始化列表构造函数
	/// @param _enElementTag 列表元素类型标签
	/// @param init 初始化列表
	/// @note 列表元素类型设置为_enElementTag代表的类型，并对进行类型检查，
	/// 如果构造的任一类型不匹配，则抛出std::invalid_argument异常
	NBT_List(NBT_TAG _enElementTag, std::initializer_list<typename List::value_type> init) : List(init), enElementTag(_enElementTag)
	{
		TestInit();
	}

	/// @brief 初始化列表构造函数
	/// @param _NoCheck 取消检查标记
	/// @param _enElementTag 列表元素类型标签
	/// @param init 初始化列表
	/// @note 不进行类型检查
	NBT_List(NoCheck_T _NoCheck, NBT_TAG _enElementTag, std::initializer_list<typename List::value_type> init) : List(init), enElementTag(_enElementTag)
	{}

	/// @brief 初始化列表构造函数
	/// @param init 初始化列表
	/// @note 根据第一个元素的类型作为列表元素类型，并进行类型检查，
	/// 如果构造的类型不匹配，则抛出std::invalid_argument异常
	NBT_List(std::initializer_list<typename List::value_type> init) : List(init), enElementTag(List::empty() ? NBT_TAG::End : List::front().GetTag())
	{
		TestInit();
	}

	/// @brief 初始化列表构造函数
	/// @param _NoCheck 取消检查标记
	/// @param init 初始化列表
	/// @note 不进行类型检查
	NBT_List(NoCheck_T _NoCheck, std::initializer_list<typename List::value_type> init) : List(init), enElementTag(List::empty() ? NBT_TAG::End : List::front().GetTag())
	{}

	/// @brief 默认构造函数
	NBT_List(void) = default;
	/// @brief 析构函数
	~NBT_List(void)
	{
		Clear();
	}

	/// @brief 移动构造函数
	/// @param _Move 要移动的源对象
	NBT_List(NBT_List &&_Move) noexcept
		:List(std::move(_Move)),
		 enElementTag(std::move(_Move.enElementTag))
	{
		_Move.enElementTag = NBT_TAG::End;
	}

	/// @brief 拷贝构造函数
	/// @param _Copy 要拷贝的源对象
	NBT_List(const NBT_List &_Copy)
		:List(_Copy),
		enElementTag(_Copy.enElementTag)
	{}

	/// @brief 移动赋值运算符
	/// @param _Move 要移动的源对象
	/// @return 当前对象的引用
	NBT_List &operator=(NBT_List &&_Move) noexcept
	{
		List::operator=(std::move(_Move));

		enElementTag = std::move(_Move.enElementTag);
		_Move.enElementTag = NBT_TAG::End;

		return *this;
	}

	/// @brief 拷贝赋值运算符
	/// @param _Copy 要拷贝的源对象
	/// @return 当前对象的引用
	NBT_List &operator=(const NBT_List &_Copy)
	{
		List::operator=(_Copy);

		enElementTag = _Copy.enElementTag;

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
		return enElementTag == _Right.enElementTag &&
			(const List &)*this == (const List &)_Right;
	}

	/// @brief 不等比较运算符
	/// @param _Right 要比较的右操作数
	/// @return 是否不相等
	bool operator!=(const NBT_List &_Right) const noexcept
	{
		return enElementTag != _Right.enElementTag ||
			(const List &)*this != (const List &)_Right;
	}

	/// @brief 三路比较运算符
	/// @param _Right 要比较的右操作数
	/// @return 比较结果，通过std::partial_ordering返回
	/// @note 列表会首先比较当前存储的类型是否与目标一致，不一致则返回Tag顺序的比较结果，
	/// 否则返回列表元素顺序的比较结果
	std::partial_ordering operator<=>(const NBT_List &_Right) const noexcept
	{
		if (auto cmp = enElementTag <=> _Right.enElementTag; cmp != 0)
		{
			return cmp;
		}

		return (const List &)*this <=> (const List &)_Right;
	}

	/// @name 暴露父类迭代器接口
	/// @brief 继承底层容器的迭代器和访问接口
	/// @note 请不要使用这些API修改list内部对象的类型（注意是类型而非值），
	/// 这些接口无法简单的进行封装并检查用户对类型的操作
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

	/// @brief 设置列表元素类型
	/// @tparam bNoCheck 是否跳过检查，作用与NoCheck_T相同
	/// @param tagNewValue 新的元素类型
	/// @return 如果bNoCheck为false，返回设置是否成功，否则返回void
	/// @note 在bNoCheck为false的情况下，当且仅当list为空函数才能成功，否则修改失败，
	/// 如果bNoCheck为true则必然成功，所以函数返回void，则用于强制修改列表元素类型
	template <bool bNoCheck = false>
	std::conditional_t<bNoCheck, void, bool> SetTag(NBT_TAG tagNewValue) noexcept
	{
		if (!bNoCheck)
		{
			if (!List::empty())
			{
				return false;
			}
		}

		enElementTag = tagNewValue;

		if (!bNoCheck)
		{
			return true;
		}
	}

	/// @brief 获取列表存储的元素类型
	/// @return 当前列表存储的元素类型
	NBT_TAG GetTag(void) const noexcept
	{
		return enElementTag;
	}

	/// @name 查询接口
	/// @brief 提供一组接口用于对list不同元素的访问
	/// @note 请不要使用这些API修改list内部对象的类型（注意是类型而非值），
	/// 这些接口无法简单的进行封装并检查用户对类型的操作
	/// @{

	/// @brief 根据位置获取值
	/// @param szPos 要查找的位置
	/// @return 位置对应的值的引用
	/// @note 如果位置不存在则抛出异常，请参考std::vector对于at的描述
	typename List::value_type &Get(const typename List::size_type &szPos)
	{
		return List::at(szPos);
	}

	/// @brief 根据位置获取值
	/// @param szPos 要查找的位置
	/// @return 位置对应的值的常量引用
	/// @note 如果位置不存在则抛出异常，请参考std::vector对于at的描述
	const typename List::value_type &Get(const typename List::size_type &szPos) const
	{
		return List::at(szPos);
	}

	/// @brief 获取列表开头的元素
	/// @return 开头的元素的引用
	/// @note 如果当前列表为空，行为未定义，请参考std::vector对于front的描述
	typename List::value_type &Front(void) noexcept
	{
		return List::front();
	}

	/// @brief 获取列表开头的元素
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

	/// @brief 获取列表最后的元素
	/// @return 最后的元素的引用
	/// @note 如果当前列表为空，行为未定义，请参考std::vector对于back的描述
	const typename List::value_type &Back(void) const noexcept
	{
		return List::back();
	}

	/// @}

	/// @name 插入接口
	/// @brief 提供一组接口用于对list进行元素插入
	/// @todo 提供范围插入等功能
	
	/// @brief 在指定位置的前面插入元素
	/// @tparam bNoCheck 是否跳过类型检查，作用与NoCheck_T相同
	/// @tparam V 元素值类型
	/// @param szPos 插入位置
	/// @param vTagVal 要插入的值
	/// @return 如果bNoCheck为false，返回包含插入元素所在位置的迭代器和是否操作成功的bool值的pair，否则仅返回迭代器
	/// @note 在bNoCheck为false的情况下，当且仅当插入的类型与当前列表存储的类型一致，同时szPos在合法范围内，函数成功，否则失败，
	/// 如果bNoCheck为true则必然成功，所以函数仅返回迭代器
	/// @note 在bNoCheck为false的情况下，如果当前列表标签为空，则列表存储的类型自动变为当前元素类型，
	/// 如果bNoCheck为true，则列表不会设置当前元素类型，请使用SetTag来变更元素类型或放弃维护元素类型
	template <bool bNoCheck = false, typename V>
	std::conditional_t<bNoCheck, typename List::iterator, std::pair<typename List::iterator, bool>> Add(size_t szPos, V &&vTagVal)
	{
		if (!bNoCheck)
		{
			if (szPos > List::size())
			{
				return std::pair{ List::end(),false };
			}
			if (!TestAndSetType(vTagVal))
			{
				return std::pair{ List::end(),false };
			}
		}

		//插入
		typename List::iterator itRet = List::emplace(List::begin() + szPos, std::forward<V>(vTagVal));

		if (!bNoCheck)
		{
			return std::pair{ itRet,true };
		}
		else
		{
			return itRet;
		}
	}

	/// @brief 在列表头部插入元素
	/// @tparam bNoCheck 是否跳过类型检查，作用与NoCheck_T相同
	/// @tparam V 元素值类型
	/// @param vTagVal 要插入的值
	/// @return 如果bNoCheck为false，返回包含插入元素所在位置的迭代器和是否操作成功的bool值的pair，否则仅返回迭代器
	/// @note 在bNoCheck为false的情况下，当且仅当插入的类型与当前列表存储的类型一致，同时szPos在合法范围内，函数成功，否则失败，
	/// 如果bNoCheck为true则必然成功，所以函数仅返回迭代器
	/// @note 在bNoCheck为false的情况下，如果当前列表标签为空，则列表存储的类型自动变为当前元素类型，
	/// 如果bNoCheck为true，则列表不会设置当前元素类型，请使用SetTag来变更元素类型或放弃维护元素类型
	template <bool bNoCheck = false, typename V>
	std::conditional_t<bNoCheck, typename List::iterator, std::pair<typename List::iterator, bool>> AddFront(V &&vTagVal)
	{
		if constexpr (!bNoCheck)
		{
			if (!TestAndSetType(vTagVal))
			{
				return std::pair{ List::end(),false };
			}
		}

		//插入
		List::emplace(List::begin(), std::forward<V>(vTagVal));

		if constexpr (!bNoCheck)
		{
			return std::pair{ List::begin(),true };
		}
		else
		{
			return List::begin();
		}
	}

	/// @brief 在列表末尾插入元素
	/// @tparam bNoCheck 是否跳过类型检查，作用与NoCheck_T相同
	/// @tparam V 元素值类型
	/// @param vTagVal 要插入的值
	/// @return 如果bNoCheck为false，返回包含插入元素所在位置的迭代器和是否操作成功的bool值的pair，否则仅返回迭代器
	/// @note 在bNoCheck为false的情况下，当且仅当插入的类型与当前列表存储的类型一致，同时szPos在合法范围内，函数成功，否则失败，
	/// 如果bNoCheck为true则必然成功，所以函数仅返回迭代器
	/// @note 在bNoCheck为false的情况下，如果当前列表标签为空，则列表存储的类型自动变为当前元素类型，
	/// 如果bNoCheck为true，则列表不会设置当前元素类型，请使用SetTag来变更元素类型或放弃维护元素类型
	template <bool bNoCheck = false, typename V>
	std::conditional_t<bNoCheck, typename List::iterator, std::pair<typename List::iterator, bool>> AddBack(V &&vTagVal)
	{
		if constexpr (!bNoCheck)
		{
			if (!TestAndSetType(vTagVal))
			{
				return std::pair{ List::end(),false };
			}
		}

		//插入
		List::emplace_back(std::forward<V>(vTagVal));

		if constexpr (!bNoCheck)
		{
			return std::pair{ List::end() - 1,true };
		}
		else
		{
			return List::end() - 1;
		}
	}

	/// @brief 设置（替换）指定位置的元素
	/// @tparam bNoCheck 是否跳过类型检查，作用与NoCheck_T相同
	/// @tparam V 元素值类型
	/// @param szPos 要设置的位置
	/// @param vTagVal 要设置的值
	/// @return 如果bNoCheck为false，返回包含插入元素所在位置的迭代器和是否操作成功的bool值的pair，否则仅返回迭代器
	/// @note 在bNoCheck为false的情况下，当且仅当插入的类型与当前列表存储的类型一致同时szPos在合法范围内，函数成功，否则失败，
	/// 如果bNoCheck为true则必然成功，所以函数仅返回迭代器
	/// @note 在bNoCheck为false的情况下，如果当前列表标签为空，，则列表存储的类型自动变为当前元素类型，
	/// 如果bNoCheck为true，则列表不会设置当前元素类型，请使用SetTag来变更元素类型或放弃维护元素类型
	template <bool bNoCheck = false, typename V>
	std::conditional_t<bNoCheck, typename List::iterator, std::pair<typename List::iterator, bool>> Set(size_t szPos, V &&vTagVal)
	{
		if constexpr (!bNoCheck)
		{
			if (szPos > List::size())
			{
				return std::pair{ List::end(),false };
			}
			if (!TestAndSetType(vTagVal))
			{
				return std::pair{ List::end(),false };
			}
		}

		List::operator[](szPos) = std::forward<V>(vTagVal);

		if constexpr (!bNoCheck)
		{
			return std::pair{ List::begin() + szPos,true };
		}
		else
		{
			return List::begin() + szPos;
		}
	}

	/// @brief 删除指定位置的元素
	/// @tparam bNoCheck 是否跳过索引检查
	/// @param szPos 要删除的位置
	/// @return 如果bNoCheck为false，返回删除是否成功，否则直接返回void
	/// @note bNoCheck为true则用户需保证szPos索引范围安全
	template <bool bNoCheck = false>
	std::conditional_t<bNoCheck, void, bool> Remove(size_t szPos)
	{
		if constexpr (!bNoCheck)
		{
			if (szPos > List::size())
			{
				return false;
			}
		}

		List::erase(List::begin() + szPos);//这个没必要返回结果，直接丢弃

		if (Empty())
		{
			enElementTag = NBT_TAG::End;//清除类型
		}

		if constexpr (!bNoCheck)
		{
			return true;
		}
	}

	/// @brief 清空所有元素
	/// @note 元素清空后，列表允许直接插入任意类型的元素
	void Clear(void)
	{
		List::clear();
		enElementTag = NBT_TAG::End;
	}

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

	/// @brief 调整容器大小
	/// @param szNewSize 新的容器大小
	void Resize(size_t szNewSize)
	{
		return List::resize(szNewSize);
	}

	/// @brief 缩减容器容量以匹配大小
	void ShrinkToFit(void)
	{
		return List::shrink_to_fit();
	}

	/// @brief 拷贝合并另一个NBT_List的内容
	/// @tparam bNoCheck 是否跳过另一个列表中的类型检查
	/// @param _Copy 要合并的源对象
	/// @return 如果bNoCheck为false，返回合并是否成功；否则返回void
	/// @note 在bNoCheck为false的情况下，只要任一列表标签为空或两个列表元素类型相同，则成功，否则失败，
	/// 如果bNoCheck为true，则必然成功，所以函数返回void
	/// @note 在bNoCheck为false的情况下，如果当前列表标签为空，则列表存储的类型自动变为当前元素类型，
	/// 如果bNoCheck为true，则列表不会设置当前元素类型，请使用SetTag来变更元素类型或放弃维护元素类型
	template <bool bNoCheck = false>
	std::conditional_t<bNoCheck, void, bool> Merge(const NBT_List &_Copy)
	{
		if constexpr (!bNoCheck)
		{
			if (enElementTag != NBT_TAG::End &&
				_Copy.enElementTag != NBT_TAG::End &&
				enElementTag != _Copy.enElementTag)
			{
				return false;
			}

			//替换当前类型
			if (enElementTag == NBT_TAG::End)
			{
				enElementTag = _Copy.enElementTag;
			}
		}

		List::insert(List::end(), _Copy.begin(), _Copy.end());

		if constexpr (!bNoCheck)
		{
			return true;
		}
	}

	/// @brief 移动合并另一个NBT_List的内容
	/// @tparam bNoCheck 是否跳过另一个列表中的类型检查
	/// @param _Move 要合并的源对象
	/// @return 如果bNoCheck为false，返回合并是否成功；否则返回void
	/// @note 在bNoCheck为false的情况下，只要任一列表标签为空或两个列表元素类型相同，则成功，否则失败，
	/// 如果bNoCheck为true，则必然成功，所以函数返回void
	/// @note 在bNoCheck为false的情况下，如果当前列表标签为空，则列表存储的类型自动变为当前元素类型，
	/// 如果bNoCheck为true，则列表不会设置当前元素类型，请使用SetTag来变更元素类型或放弃维护元素类型
	/// @note 如果函数失败，不会进行移动
	template <bool bNoCheck = false>
	std::conditional_t<bNoCheck, void, bool> Merge(NBT_List &&_Move)
	{
		if constexpr (!bNoCheck)
		{
			if (enElementTag != NBT_TAG::End &&
				_Move.enElementTag != NBT_TAG::End &&
				enElementTag != _Move.enElementTag)
			{
				return false;
			}

			//替换当前类型
			if (enElementTag == NBT_TAG::End)
			{
				enElementTag = _Move.enElementTag;
			}
		}

		List::insert(List::end(), std::make_move_iterator(_Move.begin()), std::make_move_iterator(_Move.end()));

		if constexpr (!bNoCheck)
		{
			return true;
		}
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
 @brief 获取指定位置的 type 类型数据
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
 @brief 获取列表第一个 type 类型数据
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
 @brief 获取列表最后一个 type 类型数据
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
 @tparam bNoCheck 是否跳过类型检查，作用与NoCheck_T相同
 @param szPos 插入位置
 @param vTagVal 要插入的 type 类型值
 @return 如果bNoCheck为false，返回包含迭代器和bool值的pair；否则返回迭代器
 @note 通用类型函数Add的代理，具体行为参考Add函数的说明
 */\
template <bool bNoCheck = false>\
std::conditional_t<bNoCheck, typename List::iterator, std::pair<typename List::iterator, bool>> Add##type(size_t szPos, const typename NBT_Type::type &vTagVal)\
{\
	return Add<bNoCheck>(szPos, vTagVal);\
}\
\
/**
 @brief 在指定位置插入 type 类型数据（移动）
 @tparam bNoCheck 是否跳过类型检查，作用与NoCheck_T相同
 @param szPos 插入位置
 @param vTagVal 要插入的 type 类型值
 @return 如果bNoCheck为false，返回包含迭代器和bool值的pair；否则返回迭代器
 @note 通用类型函数Add的代理，具体行为参考Add函数的说明
 */\
template <bool bNoCheck = false>\
std::conditional_t<bNoCheck, typename List::iterator, std::pair<typename List::iterator, bool>> Add##type(size_t szPos, typename NBT_Type::type &&vTagVal)\
{\
	return Add<bNoCheck>(szPos, std::move(vTagVal));\
}\
\
/**
 @brief 在列表头部插入 type 类型数据（拷贝）
 @tparam bNoCheck 是否跳过类型检查，作用与NoCheck_T相同
 @param vTagVal 要插入的 type 类型值
 @return 如果bNoCheck为false，返回包含迭代器和bool值的pair；否则返回迭代器
 @note 通用类型函数AddFront的代理，具体行为参考AddFront函数的说明
 */\
template <bool bNoCheck = false>\
std::conditional_t<bNoCheck, typename List::iterator, std::pair<typename List::iterator, bool>> AddFront##type(const typename NBT_Type::type &vTagVal)\
{\
	return AddFront<bNoCheck>(vTagVal); \
}\
\
/**
 @brief 在列表头部插入 type 类型数据（移动）
 @tparam bNoCheck 是否跳过类型检查，作用与NoCheck_T相同
 @param vTagVal 要插入的 type 类型值
 @return 如果bNoCheck为false，返回包含迭代器和bool值的pair；否则返回迭代器
 @note 通用类型函数AddFront的代理，具体行为参考AddFront函数的说明
 */\
template <bool bNoCheck = false>\
std::conditional_t<bNoCheck, typename List::iterator, std::pair<typename List::iterator, bool>> AddFront##type(typename NBT_Type::type &&vTagVal)\
{\
	return AddFront<bNoCheck>(std::move(vTagVal));\
}\
\
/**
 @brief 在列表末尾插入 type 类型数据（拷贝）
 @tparam bNoCheck 是否跳过类型检查，作用与NoCheck_T相同
 @param vTagVal 要插入的 type 类型值
 @return 如果bNoCheck为false，返回包含迭代器和bool值的pair；否则返回迭代器
 @note 通用类型函数AddBack的代理，具体行为参考AddBack函数的说明
 */\
template <bool bNoCheck = false>\
std::conditional_t<bNoCheck, typename List::iterator, std::pair<typename List::iterator, bool>> AddBack##type(const typename NBT_Type::type &vTagVal)\
{\
	return AddBack<bNoCheck>(vTagVal);\
}\
\
/**
 @brief 在列表末尾插入 type 类型数据（移动）
 @tparam bNoCheck 是否跳过类型检查，作用与NoCheck_T相同
 @param vTagVal 要插入的 type 类型值
 @return 如果bNoCheck为false，返回包含迭代器和bool值的pair；否则返回迭代器
 @note 通用类型函数AddBack的代理，具体行为参考AddBack函数的说明
 */\
template <bool bNoCheck = false>\
std::conditional_t<bNoCheck, typename List::iterator, std::pair<typename List::iterator, bool>> AddBack##type(typename NBT_Type::type &&vTagVal)\
{\
	return AddBack<bNoCheck>(std::move(vTagVal));\
}\
\
/**
 @brief 设置指定位置的 type 类型数据（拷贝）
 @tparam bNoCheck 是否跳过类型检查，作用与NoCheck_T相同
 @param szPos 要设置的位置
 @param vTagVal 要设置的 type 类型值
 @return 如果bNoCheck为false，返回包含迭代器和bool值的pair；否则返回迭代器
 @note 通用类型函数Set的代理，具体行为参考Set函数的说明
 */\
template <bool bNoCheck = false>\
std::conditional_t<bNoCheck, typename List::iterator, std::pair<typename List::iterator, bool>> Set##type(size_t szPos, const typename NBT_Type::type &vTagVal)\
{\
	return Set<bNoCheck>(szPos, vTagVal);\
}\
\
/**
 @brief 设置指定位置的 type 类型数据（移动）
 @tparam bNoCheck 是否跳过类型检查，作用与NoCheck_T相同
 @param szPos 要设置的位置
 @param vTagVal 要设置的 type 类型值
 @return 如果bNoCheck为false，返回包含迭代器和bool值的pair；否则返回迭代器
 @note 通用类型函数Set的代理，具体行为参考Set函数的说明
 */\
template <bool bNoCheck = false>\
std::conditional_t<bNoCheck, typename List::iterator, std::pair<typename List::iterator, bool>> Set##type(size_t szPos, typename NBT_Type::type &&vTagVal)\
{\
	return Set<bNoCheck>(szPos, std::move(vTagVal));\
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