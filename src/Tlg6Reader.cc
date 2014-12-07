#include <stdexcept>
#include "Tlg6Reader.h"

const std::string Tlg6Reader::get_magic() const
{
	return std::string("\x54\x4c\x47\x36\x2e\x30\x00\x72\x61\x77\x1a", 11);
}

const Image Tlg6Reader::read_raw_data(std::ifstream &ifs) const
{
	throw std::runtime_error(
		"Not implemented! Please send sample files to rr- on github.");
}
