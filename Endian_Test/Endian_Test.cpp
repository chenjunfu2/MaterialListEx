#include "..\MaterialListEx\NBT_Endian.hpp"

#include <stdio.h>


void PrintBool(bool b)
{
	printf("%s\n", b ? "true" : "false");
}

#define U64VAL (uint64_t)0x12'34'56'78'9A'BC'DE'F0
#define U32VAL (uint32_t)0x12'34'56'78
#define U16VAL (uint16_t)0x12'34
#define U8VAL (uint8_t)0x12

int main(void)
{

	PrintBool(NBT_Endian::ByteSwapAny(U8VAL) == U8VAL);
	PrintBool(NBT_Endian::ByteSwapAny(U16VAL) == NBT_Endian::ByteSwap16(U16VAL));
	PrintBool(NBT_Endian::ByteSwapAny(U32VAL) == NBT_Endian::ByteSwap32(U32VAL));
	PrintBool(NBT_Endian::ByteSwapAny(U64VAL) == NBT_Endian::ByteSwap64(U64VAL));
	
	return 0;
}