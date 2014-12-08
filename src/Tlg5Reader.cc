#include <fstream>
#include <map>
#include <memory>
#include <stdexcept>
#include "LzssCompressionState.h"
#include "LzssCompressor.h"
#include "Tlg5Reader.h"

namespace
{
	struct Tlg5Header
	{
		uint8_t channel_count;
		uint32_t image_width;
		uint32_t image_height;
		uint32_t block_height;

		void read(std::ifstream &ifs);
	};

	void Tlg5Header::read(std::ifstream &ifs)
	{
		ifs.read((char*) &channel_count, 1);
		ifs.read((char*) &image_width, 4);
		ifs.read((char*) &image_height, 4);
		ifs.read((char*) &block_height, 4);
	}

	struct Tlg5BlockInfo
	{
		uint8_t mark;
		uint32_t block_size;
		std::unique_ptr<uint8_t> block_data = nullptr;

		void read(std::ifstream &ifs);
		void decompress(
			const Tlg5Header &header,
			LzssCompressionState &compression_state);
	};

	void Tlg5BlockInfo::read(std::ifstream &ifs)
	{
		ifs.read((char*) &mark, 1);
		ifs.read((char*) &block_size, 4);
		block_data = std::unique_ptr<uint8_t>(new uint8_t[block_size]);
		ifs.read((char*) block_data.get(), block_size);
	}

	void Tlg5BlockInfo::decompress(
		const Tlg5Header &header,
		LzssCompressionState &compression_state)
	{
		auto new_data = new uint8_t[header.image_width * header.block_height];

		LzssCompressor::decompress(
			compression_state,
			block_data.get(),
			block_size,
			new_data);

		block_data = std::unique_ptr<uint8_t>(new_data);
	}

	std::map<int, Tlg5BlockInfo> read_channel_data(
		const Tlg5Header &header,
		LzssCompressionState &compression_state,
		std::ifstream &ifs)
	{
		std::map<int, Tlg5BlockInfo> map;
		for (int channel = 0; channel < header.channel_count; channel ++)
		{
			Tlg5BlockInfo block_info;
			block_info.read(ifs);
			if (!block_info.mark)
			{
				block_info.decompress(header, compression_state);
			}
			map[channel] = std::move(block_info);
		}
		return map;
	}

	void load_pixel_block_row(
		const Tlg5Header &header,
		std::map<int, Tlg5BlockInfo> channel_data,
		int block_y,
		uint32_t *pixels)
	{
		uint32_t max_y = block_y + header.block_height;
		if (max_y > header.image_height)
			max_y = header.image_height;
		bool use_alpha = header.channel_count == 4;

		for (uint32_t y = block_y; y < max_y; y ++)
		{
			uint8_t prev_red = 0;
			uint8_t prev_green = 0;
			uint8_t prev_blue = 0;
			uint8_t prev_alpha = 0;

			int block_y_shift = (y - block_y) * header.image_width;
			int prev_y_shift = (y - 1) * header.image_width;
			int y_shift = y * header.image_width;
			for (uint32_t x = 0; x < header.image_width; x ++)
			{
				uint8_t red = channel_data[2].block_data.get()[block_y_shift + x];
				uint8_t green = channel_data[1].block_data.get()[block_y_shift + x];
				uint8_t blue = channel_data[0].block_data.get()[block_y_shift + x];
				uint8_t alpha = use_alpha ? channel_data[3].block_data.get()[block_y_shift + x] : 0;

				red += green;
				blue += green;

				prev_red += red;
				prev_green += green;
				prev_blue += blue;
				prev_alpha += alpha;

				uint8_t output_red = prev_red;
				uint8_t output_green = prev_green;
				uint8_t output_blue = prev_blue;
				uint8_t output_alpha = prev_alpha;

				if (y > 0)
				{
					uint32_t above = pixels[prev_y_shift + x];
					uint8_t above_red = above & 0xff;
					uint8_t above_green = above >> 8;
					uint8_t above_blue = above >> 16;
					uint8_t above_alpha = above >> 24;
					output_red += above_red;
					output_green += above_green;
					output_blue += above_blue;
					output_alpha += above_alpha;
				}

				if (!use_alpha)
					output_alpha = 0xff;

				uint32_t output =
					output_red |
					(output_green << 8) |
					(output_blue << 16) |
					(output_alpha << 24);

				pixels[y_shift + x] = output;
			}
		}
	}

	void read_pixels(const Tlg5Header &header, std::ifstream &ifs, uint32_t *pixels)
	{
		LzssCompressionState state;
		for (uint32_t y = 0; y < header.image_height; y += header.block_height)
		{
			auto channel_data = read_channel_data(header, state, ifs);
			load_pixel_block_row(header, std::move(channel_data), y, pixels);
		}
	}
}

const std::string Tlg5Reader::get_magic() const
{
	return std::string("\x54\x4c\x47\x35\x2e\x30\x00\x72\x61\x77\x1a", 11);
}

const Image Tlg5Reader::read_raw_data(std::ifstream &ifs) const
{
	Tlg5Header header;
	header.read(ifs);
	if (header.channel_count != 3 && header.channel_count != 4)
	{
		throw std::runtime_error("Unsupported channel count");
	}

	auto block_count = (header.image_height - 1) / header.block_height + 1;
	ifs.seekg(4 * block_count, ifs.cur);

	Image image;
	image.width = header.image_width;
	image.height = header.image_height;
	image.pixels = new uint32_t[image.width * image.height];
	read_pixels(header, ifs, image.pixels);
	return image;
}
