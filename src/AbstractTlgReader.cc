#include <cstring>
#include <fstream>
#include <stdexcept>
#include "AbstractTlgReader.h"
#include "BadMagicException.h"

const Image AbstractTlgReader::read(std::string path) const
{
	std::ifstream ifs(path, std::ifstream::in | std::ifstream::binary);
	if (!ifs)
		throw std::runtime_error("Can\'t open " + path + " for reading");

	auto magic_size = get_magic().size();
	char *actual = new char[magic_size];
	try
	{
		ifs.seekg(0, ifs.beg);
		ifs.read(actual, magic_size);
		if (std::strncmp(actual, get_magic().data(), magic_size) != 0)
		{
			throw BadMagicException();
		}
		return read_from_stream(ifs);
	}
	catch (std::exception const &e)
	{
		delete[] actual;
		ifs.close();
		throw;
	}
}
