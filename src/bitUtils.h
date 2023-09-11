//bitUtils.h - Utility functions to handle bit operations

#ifndef BITUTILS_H
#define BITUTILS_H

#include <stdint.h>

inline char GetBitB(int bit, uint8_t word)
{
	return word & (1 << bit);
}
inline char GetBitI(int bit, uint32_t word)
{
	return word & (1 << bit);
}
inline char GetBitL(int bit, uint64_t word)
{
	return word & (1 << bit);
}

inline uint8_t SetBitB(int bit, uint8_t word)
{
	return word | (1 << bit);
}
inline uint32_t SetBitI(int bit, uint32_t word)
{
	return word | (1 << bit);
}
inline uint64_t SetBitL(int bit, uint64_t word)
{
	return word | (1 << bit);
}

inline uint8_t ClearBitB(int bit, uint8_t word)
{
	return word & ~(1 << bit);
}
inline uint32_t ClearBitI(int bit, uint32_t word)
{
	return word & ~(1 << bit);
}
inline uint64_t ClearBitL(int bit, uint64_t word)
{
	return word & ~(1 << bit);
}

#endif
