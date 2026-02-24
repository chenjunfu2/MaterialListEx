#pragma once

#include "NBT_Node.hpp"

/// @file
/// @brief NBT节点类型的视图，支持指向所有NBT类型的变体视图，不持有对象

//用法和NBT_Node一致，但是只持有构造时传入对象的指针，且不持有对象
//对象随时可销毁，如果销毁后使用持有销毁对象的view则行为未定义，用户自行负责
//构造传入nullptr指针且进行使用则后果自负

/// @class NBT_Node_View
/// @brief NBT节点的视图，用于指向而不持有对象，类似于标准库的std::string与std::string_view的关系
/// @note 该类使用std::variant变体来存储指向不同类型NBT数据的指针，提供类型安全的访问接口
/// @warning 视图类不持有实际数据对象，只持有指向外部数据的指针，它也不会延长外部对象或临时对象的生命周期。如果外部数据被销毁后继续使用视图则行为未定义，用户需自行负责数据生命周期管理
/// @tparam bIsConst 模板参数，指定视图是否持有数据的const指针，为true则持有对象的const指针，为false持有对象的非const指针。
/// 这里的模板值控制的状态与类是否为const类，类似于指向const对象的指针与指向对象的const指针的区别，模板控制的是指向的对象是否不可修改，而类是否const控制的是类是否能修改指向。
/// @note 与NBT_Node强制持有对象所有权不同，NBT_Node_View提供轻量级访问方式，适用于：
/// - 函数参数传递：避免拷贝开销，或者移动的持有权转移，可以直接指向现有NBT数据对象本身
/// - 临时数据访问：在明确数据生命周期时提供零拷贝访问
/// - 统一接口处理：以变体方式兼容任意NBT类型，同时保持对原始数据的直接操作能力
template <bool bIsConst>
class NBT_Node_View
{
	template <bool _bIsConst>
	friend class NBT_Node_View;//需要设置自己为友元，这样不同模板的类实例之间才能相互访问

private:
	//类型列表展开，声明std::variant
	template <typename T>
	struct AddConstIf
	{
		using type = std::conditional<bIsConst, const T, T>::type;
	};

	template <typename T>
	using PtrType = typename AddConstIf<T>::type *;

	template <typename T>
	struct TypeListPointerToVariant;

	template <typename... Ts>
	struct TypeListPointerToVariant<NBT_Type::_TypeList<Ts...>>
	{
		using type = std::variant<PtrType<Ts>...>;//展开成指针类型
	};

	using VariantData = TypeListPointerToVariant<NBT_Type::TypeList>::type;

	//数据对象（仅持有数据的指针）
	VariantData data;
public:
	/// @brief 静态常量，表示当前视图指向的数据是否只读
	static inline constexpr bool is_const = bIsConst;



	/// @brief 通用构造函数（仅适用于非const）
	/// @tparam T 要指向的数据类型
	/// @param value 要指向的数据对象的引用
	/// @note 要求类型T必须是NBT_Type类型列表中的任意一个，且不是NBT_Node类型
	/// @note 传入的对象必须在使用期间保持有效，否则行为未定义
	template <typename T>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> &&NBT_Type::IsValidType_V<std::decay_t<T>> && !bIsConst)
	NBT_Node_View(T &value) : data(&value)
	{}

	/// @brief 通用构造函数（仅适用于const）
	/// @tparam T 要指向的数据类型
	/// @param value 要指向的数据对象的常量引用
	/// @note 要求类型T必须是NBT_Type类型列表中的任意一个，且不是NBT_Node类型
	/// @warning 传入的对象必须在使用期间保持有效，否则行为未定义
	template <typename T>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && NBT_Type::IsValidType_V<std::decay_t<T>> && bIsConst)
	NBT_Node_View(const T &value) : data(&value)
	{}

	/// @brief 从NBT_Node构造视图（仅适用于非const）
	/// @param node 要创建视图的NBT_Node对象
	/// @note 创建指向NBT_Node内部数据的视图
	/// @warning 传入的NBT_Node对象必须在使用期间保持有效，否则行为未定义
	NBT_Node_View(NBT_Node &node)
	{
		std::visit([this](auto &arg)
			{
				this->data = &arg;
			}, node.data);
	}

	/// @brief 从NBT_Node构造视图（仅适用于const）
	/// @param node 要创建视图的NBT_Node对象
	/// @note 创建指向NBT_Node内部数据的只读视图
	/// @warning 传入的NBT_Node对象必须在使用期间保持有效，否则行为未定义
	template <typename = void>//requires占位模板
	requires(bIsConst)//此构造只在const的情况下生效，因为非const的情况下不能通过const对象初始化非const指针
	NBT_Node_View(const NBT_Node &node)
	{
		std::visit([this](auto &arg)
			{
				this->data = &arg;
			}, node.data);
	}

	/// @brief 从非const视图隐式构造const视图
	/// @tparam void 占位模板参数
	/// @param _Other 用于构造的非const视图对象
	/// @note 从NBT_Node_View<false>隐式构造为NBT_Node_View<true>
	template <typename = void>//requires占位模板
	requires(bIsConst)//此构造只在const的情况下生效，不能从const视图转换到非const视图
	NBT_Node_View(const NBT_Node_View<false> &_Other)
	{
		std::visit([this](auto &arg)
			{
				this->data = arg;//此处arg为非const指针
			}, _Other.data);
	}

	/// @brief 删除临时对象构造方式，防止从临时对象构造导致悬空指针
	/// @tparam T 临时对象的类型
	/// @param _Temp 任意类型的临时对象
	/// @note 这是一个删除的构造函数，用于一定程度上防御用户通过临时对象构造
	template <typename T>
	requires(!std::is_lvalue_reference_v<T>)
	NBT_Node_View(T &&_Temp) = delete;

	/// @brief 通用设置函数（仅适用于非const）
	/// @tparam T 要指向的数据类型
	/// @param value 要指向的数据对象的引用
	/// @return 设置的值的引用
	/// @note 要求类型T必须是NBT_Type类型列表中的任意一个，且不是NBT_Node类型
	/// @note 传入的对象必须在使用期间保持有效，否则行为未定义
	template <typename T>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && NBT_Type::IsValidType_V<std::decay_t<T>> && !bIsConst)
	T &Set(T &value)
	{
		data.template emplace<PtrType<T>>(&value);
		return value;
	}

	/// @brief 通用设置函数（仅适用于const）
	/// @tparam T 要指向的数据类型
	/// @param value 要指向的数据对象的常量引用
	/// @return 设置的值的常量引用
	/// @note 要求类型T必须是NBT_Type类型列表中的任意一个，且不是NBT_Node类型
	/// @warning 传入的对象必须在使用期间保持有效，否则行为未定义
	template <typename T>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && NBT_Type::IsValidType_V<std::decay_t<T>> && bIsConst)
	const T &Set(const T &value)
	{
		data.template emplace<PtrType<T>>(&value);
		return value;
	}

	/// @brief 从NBT_Node重设视图（仅适用于非const）
	/// @param node 要创建视图的NBT_Node对象
	/// @return 设置的值的引用
	/// @note 创建指向NBT_Node内部数据的视图
	/// @warning 传入的NBT_Node对象必须在使用期间保持有效，否则行为未定义
	template <typename = void>//requires占位模板
	requires(!bIsConst)
	NBT_Node &Set(NBT_Node &node)
	{
		std::visit([this](auto &arg)
			{
				this->data = &arg;
			}, node.data);

		return node;
	}

	/// @brief 从NBT_Node重设视图（仅适用于const）
	/// @param node 要创建视图的NBT_Node对象
	/// @return 设置的值的常量引用
	/// @note 创建指向NBT_Node内部数据的只读视图
	/// @warning 传入的NBT_Node对象必须在使用期间保持有效，否则行为未定义
	template <typename = void>//requires占位模板
	requires(bIsConst)
	const NBT_Node &Set(const NBT_Node &node)
	{
		std::visit([this](auto &arg)
			{
				this->data = &arg;
			}, node.data);

		return node;
	}

	/// @brief 删除临时对象设置方式，防止从临时对象构造导致悬空指针
	/// @tparam T 临时对象的类型
	/// @param _Temp 任意类型的临时对象
	/// @note 这是一个删除的设置函数，用于一定程度上防御用户通过临时对象构造
	template <typename T>
	requires(!std::is_lvalue_reference_v<T>)
	T &Set(T &&_Temp) = delete;

	/// @brief 通用赋值设置函数（仅适用于非const）
	/// @tparam T 要指向的数据类型
	/// @param value 要指向的数据对象的引用
	/// @return 设置的值的引用
	/// @note 要求类型T必须是NBT_Type类型列表中的任意一个，且不是NBT_Node类型
	/// @note 传入的对象必须在使用期间保持有效，否则行为未定义
	template <typename T>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && NBT_Type::IsValidType_V<std::decay_t<T>> && !bIsConst)
	NBT_Node_View &operator=(T &value)
	{
		data = &value;

		return *this;
	}

	/// @brief 通用赋值设置函数（仅适用于const）
	/// @tparam T 要指向的数据类型
	/// @param value 要指向的数据对象的常量引用
	/// @return 设置的值的常量引用
	/// @note 要求类型T必须是NBT_Type类型列表中的任意一个，且不是NBT_Node类型
	/// @warning 传入的对象必须在使用期间保持有效，否则行为未定义
	template <typename T>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && NBT_Type::IsValidType_V<std::decay_t<T>> && bIsConst)
	NBT_Node_View &operator=(const T &value)
	{
		data = &value;

		return *this;
	}

	/// @brief 从NBT_Node赋值重设视图（仅适用于非const）
	/// @param node 要创建视图的NBT_Node对象
	/// @return 设置的值的引用
	/// @note 创建指向NBT_Node内部数据的视图
	/// @warning 传入的NBT_Node对象必须在使用期间保持有效，否则行为未定义
	template <typename = void>
	requires(!bIsConst)
	NBT_Node_View &operator=(NBT_Node &node)
	{
		std::visit([this](auto &arg)
			{
				this->data = &arg;
			}, node.data);

		return *this;
	}

	/// @brief 从NBT_Node赋值重设视图（仅适用于const）
	/// @param node 要创建视图的NBT_Node对象
	/// @return 设置的值的常量引用
	/// @note 创建指向NBT_Node内部数据的只读视图
	/// @warning 传入的NBT_Node对象必须在使用期间保持有效，否则行为未定义
	template <typename = void>
	requires(bIsConst)
	NBT_Node_View &operator=(const NBT_Node &node)
	{
		std::visit([this](auto &arg)
			{
				this->data = &arg;
			}, node.data);

		return *this;
	}

	/// @brief 删除临时对象赋值设置方式，防止从临时对象构造导致悬空指针
	/// @tparam T 临时对象的类型
	/// @param _Temp 任意类型的临时对象
	/// @note 这是一个删除的设置函数，用于一定程度上防御用户通过临时对象构造
	template <typename T>
	requires(!std::is_lvalue_reference_v<T>)
	NBT_Node_View &operator=(T &&_Temp) = delete;

public:
	/// @brief 默认构造函数
	/// @note 构造为空View
	NBT_Node_View() : data(PtrType<NBT_Type::End>{})
	{}

	/// @brief 默认析构函数
	/// @note 视图析构不会影响实际数据对象，只释放内部指针
	~NBT_Node_View() = default;

	/// @brief 拷贝构造函数
	/// @param _Copy 要拷贝的源视图对象
	/// @note 拷贝构造会创建指向相同数据对象的新视图，这可以让多个视图指向同一段数据
	NBT_Node_View(const NBT_Node_View &_Copy) : data(_Copy.data)
	{}

	/// @brief 移动构造函数
	/// @param _Move 要移动的源视图对象
	/// @note 移动构造会转移指针，原视图将变为无效状态
	NBT_Node_View(NBT_Node_View &&_Move) noexcept : data(std::move(_Move.data))
	{}

	/// @brief 拷贝赋值运算符
	/// @param _Copy 要拷贝的源视图对象
	/// @return 当前视图对象指向的数据的引用
	/// @note 赋值操作会使当前视图指向与源视图相同的数据对象，这可以让多个视图指向同一段数据
	NBT_Node_View &operator=(const NBT_Node_View &_Copy)
	{
		data = _Copy.data;
		return *this;
	}

	/// @brief 移动赋值运算符
	/// @param _Move 要移动的源视图对象
	/// @return 当前视图对象指向的数据的引用
	/// @note 移动赋值会转移指针，原视图将变为无效状态
	NBT_Node_View &operator=(NBT_Node_View &&_Move) noexcept
	{
		data = std::move(_Move.data);
		return *this;
	}

	/// @brief 相等比较运算符
	/// @param _Right 要比较的右操作数
	/// @return 是否相等
	/// @note 比较两个视图指向的数据内容是否相等，如果两个视图存储的类型不同则直接返回false，
	/// 否则会返回指向的对象的比较结果
	bool operator==(const NBT_Node_View &_Right) const noexcept
	{
		if (GetTag() != _Right.GetTag())
		{
			return false;
		}

		return std::visit([this](const auto *argL, const auto *argR)-> bool
			{
				using TL = const std::decay_t<decltype(argL)>;
				return *argL == *(TL)argR;
			}, this->data, _Right.data);
	}

	/// @brief 不等比较运算符
	/// @param _Right 要比较的右操作数
	/// @return 是否不相等
	/// @note 比较两个视图指向的数据内容是否不相等，如果两个视图存储的类型不同则直接返回false，
	/// 否则会返回指向的对象的比较结果
	bool operator!=(const NBT_Node_View &_Right) const noexcept
	{
		if (GetTag() != _Right.GetTag())
		{
			return true;
		}

		return std::visit([this](const auto *argL, const auto *argR)-> bool
			{
				using TL = const std::decay_t<decltype(argL)>;
				return *argL != *(TL)argR;
			}, this->data, _Right.data);
	}

	/// @brief 三路比较运算符
	/// @param _Right 要比较的右操作数
	/// @return 比较结果，通过std::partial_ordering返回
	/// @note 比较两个视图指向的数据内容，如果两个视图存储的类型不同则直接返回unordered，
	/// 否则会返回指向的对象的比较结果
	std::partial_ordering operator<=>(const NBT_Node_View &_Right) const noexcept
	{
		if (GetTag() != _Right.GetTag())
		{
			return std::partial_ordering::unordered;
		}

		return std::visit([this](const auto *argL, const auto *argR)-> std::partial_ordering
			{
				using TL = const std::decay_t<decltype(argL)>;
				return *argL <=> *(TL)argR;
			}, this->data, _Right.data);
	}

	/// @brief 获取当前视图指向的NBT类型的枚举值
	/// @return NBT类型枚举值
	NBT_TAG GetTag() const noexcept
	{
		return (NBT_TAG)data.index();//返回当前存储类型的index（0基索引，与NBT_TAG enum一一对应）
	}

	/// @brief 通过指定类型获取当前视图指向的数据对象
	/// @tparam T 要访问的数据类型
	/// @return 对指向数据的常量引用
	/// @note 如果类型不存在或当前存储的不是指定类型的指针，则抛出异常，具体请参考std::get的说明
	template<typename T>
	const T &Get() const
	{
		return *std::get<PtrType<T>>(data);
	}

	/// @brief 通过指定类型获取当前视图指向的数据对象（仅适用于非const视图）
	/// @tparam T 要访问的数据类型
	/// @return 对指向数据的引用
	/// @note 如果类型不存在或当前存储的不是指定类型的指针，则抛出异常，具体请参考std::get的说明
	template<typename T>
	requires(!bIsConst)//仅在非const的情况下可用
	T &Get()
	{
		return *std::get<PtrType<T>>(data);
	}

	/// @brief 类型判断
	/// @tparam T 要判断的数据类型
	/// @return 当前视图是否指向指定类型的数据
	template<typename T>
	bool TypeHolds() const
	{
		return std::holds_alternative<PtrType<T>>(data);
	}

	/// @brief 为空判断
	/// @return 如果存储空值则返回true，否则返回false
	bool IsEmpty() const
	{
		return TypeHolds<NBT_Type::End>() &&
			std::get<PtrType<NBT_Type::End>>(data) == PtrType<NBT_Type::End>{};
	}

	/// @brief 设置为空
	void SetEmpty()
	{
		data.template emplace<PtrType<NBT_Type::End>>(PtrType<NBT_Type::End>{});
	}

//针对每种类型生成一个方便的函数
//通过宏定义批量生成

/// @def TYPE_GET_FUNC(type)
/// @brief 不同类型名接口生成宏
/// @note 用户不应该使用此宏（实际上宏已在使用后取消定义），标注仅为消除doxygen警告
#define TYPE_GET_FUNC(type)\
/**
 @brief 获取当前视图指向的 type 类型的数据
 @return 对指定类型数据的常量引用
 @note 如果类型不存在或当前指向的不是 type 类型，则抛出异常，具体请参考std::get的说明
 */\
const NBT_Type::type &Get##type() const\
{\
	return *std::get<PtrType<NBT_Type::type>>(data);\
}\
\
/**
 @brief 获取当前视图指向的 type 类型的数据（仅适用于非const视图）
 @return 对指定类型数据的引用
 @note 如果类型不存在或当前指向的不是 type 类型，则抛出异常，具体请参考std::get的说明
 */\
template <typename = void>\
requires(!bIsConst)\
NBT_Type::type &Get##type()\
{\
	return *std::get<PtrType<NBT_Type::type>>(data);\
}\
\
/**
 @brief 检查当前视图是否指向 type 类型的数据
 @return 是否指向 type 类型
 */\
bool Is##type() const\
{\
	return std::holds_alternative<PtrType<NBT_Type::type>>(data);\
}\
/**
 @brief 友元函数：从NBT_Node_View对象中获取 type 类型的数据
 @param node 要从中获取类型的NBT_Node_View对象
 @return 对 type 类型数据的引用（根据视图的const属性决定返回常量引用或非常量引用）
 @note 如果类型不存在或当前指向的不是 type 类型，则抛出异常，具体请参考std::get的说明
 */\
friend std::conditional_t<bIsConst, const NBT_Type::type &, NBT_Type::type &> Get##type(NBT_Node_View & node)\
{\
	return node.Get##type();\
}\
\
/**
 @brief 友元函数：从NBT_Node_View对象中获取 type 类型的数据
 @param node 要从中获取类型的NBT_Node_View对象
 @return 对 type 类型数据的引用（根据视图的const属性决定返回常量引用或非常量引用）
 @note 如果类型不存在或当前指向的不是 type 类型，则抛出异常，具体请参考std::get的说明
 */\
friend std::conditional_t<bIsConst, const NBT_Type::type &, NBT_Type::type &> &Get##type(const NBT_Node_View & node)\
{\
	return node.Get##type();\
}\
\
/**
 @brief 友元函数：检查目标对象中是否指向 type 类型的数据
 @param node 要检查的对象
 @return 对象中是否指向 type 类型
*/\
friend bool Is##type(const NBT_Node_View &node)\
{\
	return node.Is##type();\
}

	/// @name 针对每种类型提供一个方便使用的函数，由宏批量生成
	/// @brief 具体作用说明：
	/// - Get开头+类型名的函数：直接获取此类型，不做任何检查，由std::get具体实现决定
	/// - Is开头 + 类型名的函数：判断当前NBT_Node_View是否指向此类型
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

/// @def TYPE_SET_FUNC(type)
/// @brief 不同类型名接口生成宏
/// @note 用户不应该使用此宏（实际上宏已在使用后取消定义），标注仅为消除doxygen警告
#define TYPE_SET_FUNC(type)\
/**
 @brief 从value设置 type 类型的视图（非const）
 @param value 要设置的值的引用
 @return 设置的值的引用
 */\
template <typename = void>\
requires(!bIsConst)\
NBT_Type::type &Set##type(NBT_Type::type &value)\
{\
	return Set<NBT_Type::type>(value); \
}\
\
/**
 @brief 从value设置 type 类型的视图（const）
 @param value 要设置的值的常量引用
 @return 设置的值的引用
 */\
template <typename = void>\
requires(bIsConst)\
const NBT_Type::type &Set##type(const NBT_Type::type &value)\
{\
	return Set<NBT_Type::type>(value); \
}\
\
/**
 @brief 删除从临时对象设置 type 类型的视图，，防止从临时对象构造导致悬空指针
 @param _Temp type 类型的临时对象
 @note 这是一个删除的设置函数，用于一定程度上防御用户通过临时对象构造
 */\
NBT_Type::type &Set##type(NBT_Type::type &&_Temp) = delete;

	/// @name 针对每种类型提供一个方便使用的函数，由宏批量生成
	/// @brief 具体作用说明：
	/// - Set开头+类型名的函数：设置node为此类型，从值或默认值设置
	/// @{
	
	TYPE_SET_FUNC(End);
	TYPE_SET_FUNC(Byte);
	TYPE_SET_FUNC(Short);
	TYPE_SET_FUNC(Int);
	TYPE_SET_FUNC(Long);
	TYPE_SET_FUNC(Float);
	TYPE_SET_FUNC(Double);
	TYPE_SET_FUNC(ByteArray);
	TYPE_SET_FUNC(IntArray);
	TYPE_SET_FUNC(LongArray);
	TYPE_SET_FUNC(String);
	TYPE_SET_FUNC(List);
	TYPE_SET_FUNC(Compound);

	/// @}

#undef TYPE_SET_FUNC
};
