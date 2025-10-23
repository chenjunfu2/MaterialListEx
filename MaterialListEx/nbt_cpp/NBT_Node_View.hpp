#pragma once

#include "NBT_Node.hpp"

//用法和NBT_Node一致，但是只持有构造时传入对象的指针，且不持有对象
//对象随时可销毁，如果销毁后使用持有销毁对象的view则行为未定义，用户自行负责
//构造传入nullptr指针且进行使用则后果自负

template <bool bIsConst>
class NBT_Node_View
{
	template <bool _bIsConst>
	friend class NBT_Node_View;//需要设置自己为友元，这样不同模板的类实例之间才能相互访问

private:
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

	VariantData data;
public:
	static inline constexpr bool is_const = bIsConst;

	// 通用构造函数（必须非const情况）
	template <typename T>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node> && !bIsConst)
	NBT_Node_View(T &value) : data(&value)
	{
		static_assert(NBT_Type::IsValidType_V<std::decay_t<T>>, "Invalid type for NBT node view");
	}

	// 通用构造函数
	template <typename T>
	requires(!std::is_same_v<std::decay_t<T>, NBT_Node>)
	NBT_Node_View(const T &value) : data(&value)
	{
		static_assert(NBT_Type::IsValidType_V<std::decay_t<T>>, "Invalid type for NBT node view");
	}

	//从NBT_Node构造（必须非const情况）
	template <typename = void>//requires占位模板
	requires(!bIsConst)
	NBT_Node_View(NBT_Node &node)
	{
		std::visit([this](auto &arg)
			{
				this->data = &arg;
			}, node.data);
	}

	//从NBT_Node构造
	NBT_Node_View(const NBT_Node &node)
	{
		std::visit([this](auto &arg)
			{
				this->data = &arg;
			}, node.data);
	}

	// 允许从非 const 视图隐式转换到 const 视图
	template <typename = void>//requires占位模板
	requires(bIsConst)
	NBT_Node_View(const NBT_Node_View<false> &other)
	{
		std::visit([this](auto &arg)
			{
				this->data = arg;//此处arg为非const指针
			}, other.data);
	}

private:
	// 隐藏默认构造（TAG_End）
	NBT_Node_View() = default;
public:

	// 自动析构由variant处理
	~NBT_Node_View() = default;

	//拷贝构造
	NBT_Node_View(const NBT_Node_View &_NBT_Node_View) : data(_NBT_Node_View.data)
	{}

	//移动构造
	NBT_Node_View(NBT_Node_View &&_NBT_Node_View) noexcept : data(std::move(_NBT_Node_View.data))
	{}

	NBT_Node_View &operator=(const NBT_Node_View &_NBT_Node_View)
	{
		data = _NBT_Node_View.data;
		return *this;
	}

	NBT_Node_View &operator=(NBT_Node_View &&_NBT_Node_View) noexcept
	{
		data = std::move(_NBT_Node_View.data);
		return *this;
	}


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

	//获取标签类型
	NBT_TAG GetTag() const noexcept
	{
		return (NBT_TAG)data.index();//返回当前存储类型的index（0基索引，与NBT_TAG enum一一对应）
	}

	//类型安全访问
	template<typename T>
	const T &GetData() const
	{
		return *std::get<PtrType<T>>(data);
	}

	template<typename T>
	requires(!bIsConst)
	T &GetData()
	{
		return *std::get<PtrType<T>>(data);
	}

	// 类型检查
	template<typename T>
	bool TypeHolds() const
	{
		return std::holds_alternative<PtrType<T>>(data);
	}

	//针对每种类型重载一个方便的函数
	/*
		纯类型名函数：直接获取此类型，不做任何检查，由标准库std::get具体实现决定
		Is开头的类型名函数：判断当前NBT_Node是否为此类型
		纯类型名函数带参数版本：查找当前Compound指定的Name并转换到类型引用返回，不做检查，具体由标准库实现定义
		Has开头的类型名函数带参数版本：查找当前Compound是否有特定Name的Tag，并返回此Name的Tag（转换到指定类型）的指针
	*/
#define TYPE_GET_FUNC(type)\
const NBT_Type::type &Get##type() const\
{\
	return *std::get<PtrType<NBT_Type::type>>(data);\
}\
\
template <typename = void>\
requires(!bIsConst)\
NBT_Type::type &Get##type()\
{\
	return *std::get<PtrType<NBT_Type::type>>(data);\
}\
\
bool Is##type() const\
{\
	return std::holds_alternative<PtrType<NBT_Type::type>>(data);\
}\
friend std::conditional_t<bIsConst, const NBT_Type::type &, NBT_Type::type &> Get##type(NBT_Node_View & node)\
{\
	return node.Get##type();\
}\
\
friend const NBT_Type::type &Get##type(const NBT_Node_View & node)\
{\
	return node.Get##type();\
}

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

#undef TYPE_GET_FUNC
};