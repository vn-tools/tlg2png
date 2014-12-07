#include <png.h>
#include <fstream>
#include <memory>
#include <stdexcept>
#include "BadMagicException.h"
#include "TlgConverter.h"
#include "AbstractTlgReader.h"
#include "Tlg0Reader.h"
#include "Tlg5Reader.h"
#include "Tlg6Reader.h"

const Image TlgConverter::read(std::string path) const
{
	std::ifstream ifs(path, std::ifstream::in | std::ifstream::binary);
	if (!ifs)
		throw std::runtime_error("Can\'t open " + path + " for reading");

	try
	{
		auto reader = AbstractTlgReader::choose_reader(ifs);
		auto ret = reader->read(path);
		ifs.close();
		return ret;
	}
	catch (std::exception const &e)
	{
		ifs.close();
		throw;
	}
}

void TlgConverter::save(const Image image, std::string path) const
{
	FILE *fp = fopen(path.c_str(), "wb");
	if (fp == nullptr)
		throw std::runtime_error("Could not open " + path + " for writing");

	png_structp png_ptr = nullptr;
	png_infop info_ptr = nullptr;
	try
	{
		auto png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (png_ptr == nullptr)
			throw std::runtime_error("Could not allocate write struct\n");

		auto info_ptr = png_create_info_struct(png_ptr);
		if (info_ptr == nullptr)
			throw std::runtime_error("Could not allocate info struct\n");

		png_init_io(png_ptr, fp);

		if (setjmp(png_jmpbuf(png_ptr)))
			throw std::runtime_error("Error during PNG creation\n");

		// Write header (8 bit colour depth)
		png_set_IHDR(
			png_ptr,
			info_ptr,
			image.width,
			image.height,
			8,
			PNG_COLOR_TYPE_RGBA,
			PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE,
			PNG_FILTER_TYPE_BASE);

		// 0 = no compression, 9 = max compression
		// 1 produces good file size and is still fast.
		png_set_compression_level(png_ptr, 1);

		png_write_info(png_ptr, info_ptr);

		for (uint32_t y = 0; y < image.height; y ++)
		{
			png_write_row(png_ptr, (png_bytep) &image.pixels[y * image.width]);
		}

		png_write_end(png_ptr, nullptr);
	}
	catch (std::exception const &e)
	{
		if (info_ptr != nullptr)
			png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
		if (png_ptr != nullptr)
			png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

		fclose(fp);
		throw;
	}
}
