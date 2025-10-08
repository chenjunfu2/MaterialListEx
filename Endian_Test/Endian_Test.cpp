#include "../MaterialListEx/nbt/NBT_Endian.hpp"

#include <stdio.h>


void PrintBool(bool b)
{
	printf("%s\n", b ? "true" : "false");
}

#define U64VAL (uint64_t)0x12'34'56'78'9A'BC'DE'F0
#define _U64VAL (uint64_t)0xF0'DE'BC'9A'78'56'34'12

#define U32VAL (uint32_t)0x12'34'56'78
#define _U32VAL (uint32_t)0x78'56'34'12

#define U16VAL (uint16_t)0x12'34
#define _U16VAL (uint16_t)0x34'12

#define U8VAL (uint8_t)0x12
#define _U8VAL (uint8_t)0x12

int main(void)
{

	PrintBool(NBT_Endian::ByteSwapAny(U8VAL) == _U8VAL);
	PrintBool(NBT_Endian::ByteSwapAny(U16VAL) == _U16VAL);
	PrintBool(NBT_Endian::ByteSwapAny(U32VAL) == _U32VAL);
	PrintBool(NBT_Endian::ByteSwapAny(U64VAL) == _U64VAL);
	
	return 0;
}