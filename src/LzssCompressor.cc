#include "LzssCompressor.h"

void LzssCompressor::decompress(
	LzssCompressionState &compression_state,
	uint8_t *input,
	size_t input_size,
	uint8_t *output)
{
	uint8_t *end = input + input_size;

	for (uint32_t i = 0; i < input_size; i ++)
		output[i] = 0;

	int flags = 0;
	while (input < end)
	{
		flags >>= 1;
		if ((flags & 0x100) != 0x100)
			flags = *input++ | 0xff00;

		if ((flags & 1) == 1)
		{
			uint8_t x0 = *input++;
			uint8_t x1 = *input++;
			int position = x0 | ((x1 & 0xf) << 8);
			int length = 3 + ((x1 & 0xf0) >> 4);
			if (length == 18)
				length += *input++;
			for (int j = 0; j < length; j ++)
			{
				uint8_t c = compression_state.text[position];
				*output ++ = c;
				compression_state.text[compression_state.offset] = c;
				compression_state.offset ++;
				compression_state.offset &= 0xfff;
				position ++;
				position &= 0xfff;
			}
		}
		else
		{
			uint8_t c = *input ++;
			*output ++ = c;
			compression_state.text[compression_state.offset] = c;
			compression_state.offset ++;
			compression_state.offset &= 0xfff;
		}
	}
}
