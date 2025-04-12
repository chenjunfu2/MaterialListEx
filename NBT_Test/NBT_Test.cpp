#include "..\MaterialListEx\NBT_Node.hpp"
#include "..\MaterialListEx\NBT_Reader.hpp"
#include "..\MaterialListEx\NBT_Writer.hpp"
#include "..\MaterialListEx\NBT_Helper.hpp"
#include "..\MaterialListEx\MUTF8_Tool.hpp"
#include "..\MaterialListEx\Windows_ANSI.hpp"
#include "..\MaterialListEx\File_Tool.hpp"
#include "..\MaterialListEx\Compression_Utils.hpp"

#include <stdio.h>


int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("Only one input file is needed\n");
		return -1;
	}

	std::string sNbtData;
	if (!ReadFile(argv[1], sNbtData))
	{
		printf("Nbt File Read Fail\n");
		return -1;
	}

	printf("NBT file read size: [%zu]\n", sNbtData.size());


	if (gzip::is_compressed(sNbtData.data(), sNbtData.size()))//如果nbt已压缩，解压，否则保持原样
	{
		sNbtData = gzip::decompress(sNbtData.data(), sNbtData.size());
		printf("NBT file decompressed size: [%lld]\n", (uint64_t)sNbtData.size());
	}
	else
	{
		printf("NBT file is not compressed\n");
	}














	return 0;
}