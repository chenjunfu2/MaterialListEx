#pragma once

#include "NBT_Node.hpp"

template <typename T>
const T &Max(const T &l, const T &r)
{
	return (l > r) ? l : r;
}

template <typename T>
const T &Min(const T &l, const T &r)
{
	return (l < r) ? l : r;
}



struct BlockPos
{
	NBT_Type::Int x{ 0 }, y{ 0 }, z{ 0 };
	//类相关函数由编译器代理生成

	BlockPos add(const BlockPos &r) const
	{
		return BlockPos{ x + r.x,y + r.y,z + r.z };
	}

	BlockPos sub(const BlockPos &r) const
	{
		return BlockPos{ x - r.x,y - r.y,z - r.z };
	}
};

static BlockPos getMinCorner(const BlockPos &l, const BlockPos &r)
{
	return BlockPos{ Min(l.x,r.x),Min(l.y,r.y),Min(l.z,r.z) };
}

static BlockPos getMaxCorner(const BlockPos &l, const BlockPos &r)
{
	return BlockPos{ Max(l.x,r.x),Max(l.y,r.y),Max(l.z,r.z) };
}

static BlockPos getRelativeEndPositionFromAreaSize(const BlockPos &size)
{
	auto x = size.x;
	auto y = size.y;
	auto z = size.z;

	x = x >= 0 ? x - 1 : x + 1;
	y = y >= 0 ? y - 1 : y + 1;
	z = z >= 0 ? z - 1 : z + 1;

	return BlockPos{ x, y, z };
}


static uint32_t numberOfLeadingZeros(uint32_t i)
{
	if (i == 0)
		return 32;
	uint32_t n = 31;
	if (i >= 1 << 16)
	{
		n -= 16; i >>= 16;
	}
	if (i >= 1 << 8)
	{
		n -= 8; i >>= 8;
	}
	if (i >= 1 << 4)
	{
		n -= 4; i >>= 4;
	}
	if (i >= 1 << 2)
	{
		n -= 2; i >>= 2;
	}
	return n - (i >> 1);
}
