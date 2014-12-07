#include <cstring>
#include <string>
#include <iostream>
#include <memory>
#include <stdexcept>
#include "Tlg0Reader.h"
#include "Tlg5Reader.h"
#include "Tlg6Reader.h"

namespace
{
	std::string extract_string(char *&ptr)
	{
		int length = 0;
		while (*ptr >= '0' && *ptr <= '9')
		{
			length *= 10;
			length += *ptr++ - '0';
		}

		if (*ptr != ':')
			return "";
		ptr ++;

		std::string value;
		for (int i = 0; i < length; i ++)
			value += *ptr ++;

		return value;
	}

	void process_tag_chunk(std::unique_ptr<char> chunk_data, size_t chunk_size)
	{
		char *start = chunk_data.get();
		char *ptr = start;
		while (ptr < start + chunk_size)
		{
			auto key = extract_string(ptr);
			ptr ++;
			auto value = extract_string(ptr);
			ptr ++;
			std::cout << "Tag found: " << key << " = " << value << std::endl;
		}
	}

	void process_chunks(std::ifstream &ifs)
	{
		char chunk_name[4];
		while (ifs.read(chunk_name, 4))
		{
			uint32_t chunk_size;
			ifs.read((char*) &chunk_size, 4);

			auto chunk_data = std::unique_ptr<char>(new char[chunk_size]);
			ifs.read(chunk_data.get(), chunk_size);
			if (std::strncmp(chunk_name, "tags", 4) == 0)
			{
				process_tag_chunk(std::move(chunk_data), chunk_size);
			}
		}
	}
}

const std::string Tlg0Reader::get_magic() const
{
	return std::string("\x54\x4c\x47\x30\x2e\x30\x00\x73\x64\x73\x1a", 11);
}

const Image Tlg0Reader::read_raw_data(std::ifstream &ifs) const
{
	uint32_t raw_data_size;
	ifs.read((char*) &raw_data_size, 4);

	auto reader = AbstractTlgReader::choose_reader(ifs);
	auto ret = reader->read_raw_data(ifs);

	process_chunks(ifs);

	return ret;
}
