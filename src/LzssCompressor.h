#ifndef LZSS_COMPRESSOR_H_
#define LZSS_COMPRESSOR_H_
#include <cstdint>
#include <cstddef>
#include "LzssCompressionState.h"

class LzssCompressor
{
	public:
		static void decompress(
			LzssCompressionState &compression_state,
			uint8_t *input,
			size_t input_size,
			uint8_t *output);
};

#endif
