#include <memory>
#include <stdexcept>
#include "LzssCompressionState.h"
#include "LzssCompressor.h"
#include "Tlg6Reader.h"

#define TLG6_W_BLOCK_SIZE 8
#define TLG6_H_BLOCK_SIZE 8

namespace
{
	struct Tlg6Header
	{
		uint8_t channel_count;
		uint8_t data_flags;
		uint8_t color_type;
		uint8_t external_golomb_table;
		uint32_t image_width;
		uint32_t image_height;
		uint32_t max_bit_length;
		uint32_t x_block_count;
		uint32_t y_block_count;

		void read(std::ifstream &ifs);
	};

	void Tlg6Header::read(std::ifstream &ifs)
	{
		ifs.read((char*) &channel_count, 1);
		ifs.read((char*) &data_flags, 1);
		ifs.read((char*) &color_type, 1);
		ifs.read((char*) &external_golomb_table, 1);
		ifs.read((char*) &image_width, 4);
		ifs.read((char*) &image_height, 4);
		ifs.read((char*) &max_bit_length, 4);

		x_block_count = ((image_width - 1)/ TLG6_W_BLOCK_SIZE) + 1;
		y_block_count = ((image_height - 1)/ TLG6_H_BLOCK_SIZE) + 1;
	}

	struct Tlg6FilterTypes
	{
		uint32_t data_size;
		std::unique_ptr<uint8_t> data = nullptr;

		void read(std::ifstream &ifs);
		void decompress(Tlg6Header const &header);
	};

	void Tlg6FilterTypes::read(std::ifstream &ifs)
	{
		ifs.read((char*) &data_size, 4);
		data = std::unique_ptr<uint8_t>(new uint8_t[data_size]);
		ifs.read((char*) data.get(), data_size);
	}

	void Tlg6FilterTypes::decompress(Tlg6Header const &header)
	{
		auto output_size = header.x_block_count * header.y_block_count;
		uint8_t *output = new uint8_t[output_size];

		LzssCompressionState compression_state;
		uint8_t *ptr = compression_state.text;
		for (int i = 0; i < 32; i ++)
		{
			for (int j = 0; j < 16; j ++)
			{
				for (int k = 0; k < 4; k ++)
					*ptr ++ = i;

				for (int k = 0; k < 4; k ++)
					*ptr ++ = j;
			}
		}

		LzssCompressor::decompress(
			compression_state,
			data.get(),
			data_size,
			output);

		data = std::unique_ptr<uint8_t>(output);
	}
}

const std::string Tlg6Reader::get_magic() const
{
	return std::string("\x54\x4c\x47\x36\x2e\x30\x00\x72\x61\x77\x1a", 11);
}

const Image Tlg6Reader::read_raw_data(std::ifstream &ifs) const
{
	Tlg6Header header;
	header.read(ifs);
	if (header.channel_count != 1
		&& header.channel_count != 3
		&& header.channel_count != 4)
	{
		throw std::runtime_error("Unsupported channel count");
	}

	Tlg6FilterTypes filter_types;
	filter_types.read(ifs);
	filter_types.decompress(header);

	throw std::runtime_error(
		"Not implemented! Please send sample files to rr- on github.");
}
