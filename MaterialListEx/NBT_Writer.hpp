#pragma once

#include "NBT_Node.hpp"

class OutputStream
{
private:
	std::string &sString;
public:
	OutputStream(std::string &_sString, size_t szStartIdx = 0) :sString(_sString)
	{
		sString.resize(szStartIdx);
	}
	~OutputStream() = default;

	void PutChar(char c)
	{
		return sString.push_back(c);
	}

	void UnPut()
	{
		sString.pop_back();
	}

	size_t GetSize() const
	{
		return sString.size();
	}

	void Reset()
	{
		sString.resize(0);
	}

	const std::string &Data() const
	{
		return sString;
	}

	std::string &Data()
	{
		return sString;
	}
};


class NBT_Writer
{






};