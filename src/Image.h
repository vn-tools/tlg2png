#ifndef IMAGE_H_
#define IMAGE_H_
#include <cstdint>

struct Image
{
	uint32_t width;
	uint32_t height;
	uint32_t *pixels;
};

#endif
