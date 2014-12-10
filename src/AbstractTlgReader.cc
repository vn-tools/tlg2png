#include <cstring>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <vector>
#include "AbstractTlgReader.h"
#include "BadMagicException.h"
#include "Tlg0Reader.h"
#include "Tlg5Reader.h"
#include "Tlg6Reader.h"

std::unique_ptr<const AbstractTlgReader>
	AbstractTlgReader::choose_reader(std::ifstream &ifs)
{
	std::vector<AbstractTlgReader*> readers
	{
		new Tlg0Reader(),
		new Tlg5Reader(),
		new Tlg6Reader(),
	};

	auto pos = ifs.tellg();
	for (auto reader : readers)
	{
		try
		{
			ifs.seekg(pos, ifs.beg);
			if (reader->is_magic_valid(ifs))
			{
				return std::unique_ptr<AbstractTlgReader>(reader);
			}
		}
		catch (BadMagicException const &e)
		{
			continue;
		}
	}

	throw std::runtime_error("Not a TLG image");
}

const Image AbstractTlgReader::read(std::string path) const
{
	std::ifstream ifs(path, std::ifstream::in | std::ifstream::binary);
	if (!ifs)
		throw std::runtime_error("Can\'t open " + path + " for reading");

	try
	{
		ifs.seekg(0, ifs.beg);
		if (!is_magic_valid(ifs))
			throw BadMagicException();
		auto ret = read_raw_data(ifs);
		ifs.close();
		return ret;
	}
	catch (std::exception const &e)
	{
		ifs.close();
		throw;
	}
}

const bool AbstractTlgReader::is_magic_valid(std::ifstream &ifs) const
{
	bool result = false;
	auto magic_size = get_magic().size();
	char *actual = new char[magic_size];
	try
	{
		ifs.read(actual, magic_size);
		if (std::strncmp(actual, get_magic().data(), magic_size) == 0)
		{
			result = true;
		}
	}
	catch (std::exception const &e)
	{
	}
	delete[] actual;
	return result;
}
