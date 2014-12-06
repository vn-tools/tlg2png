#include <Magick++.h>
#include <fstream>
#include <stdexcept>
#include "BadMagicException.h"
#include "TlgConverter.h"
#include "Tlg5Reader.h"
#include "Tlg6Reader.h"

const Image TlgConverter::read(std::string path) const
{
	Tlg5Reader tlg5reader;
	Tlg6Reader tlg6reader;
	try
	{
		return tlg5reader.read(path);
	}
	catch (BadMagicException const &e)
	{
		return tlg6reader.read(path);
	}
}

void TlgConverter::save(const Image image, std::string path) const
{
	Magick::Image magick_image(
		image.width,
		image.height,
		"RGBA",
		Magick::StorageType::CharPixel,
		image.pixels);

	magick_image.compressType(Magick::CompressionType::NoCompression);
	magick_image.write(path);
}
