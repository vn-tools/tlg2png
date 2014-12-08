#ifndef LZSS_COMPRESSION_STATE_H_
#define LZSS_COMPRESSION_STATE_H_
#include <cstdint>

struct LzssCompressionState
{
	uint8_t text[4096];
	uint16_t offset = 0;

	LzssCompressionState();
};

#endif
