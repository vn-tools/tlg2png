#include <cstring> //!
#include <iostream> //!
#include <memory>
#include <stdexcept>
#include "LzssCompressionState.h"
#include "LzssCompressor.h"
#include "Tlg6Reader.h"

namespace
{
	const int w_block_size = 8;
	const int h_block_size = 8;
	const int golomb_n_count = 4;
	const int leading_zero_table_bits = 12;
	const int leading_zero_table_size = 1 << leading_zero_table_bits;

	uint8_t leading_zero_table[leading_zero_table_size];
	uint8_t golomb_bit_length_table[golomb_n_count * 2 * 128][golomb_n_count];

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

		x_block_count = ((image_width - 1)/ w_block_size) + 1;
		y_block_count = ((image_height - 1)/ h_block_size) + 1;
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

	inline uint32_t make_gt_mask(
		uint32_t const &a,
		uint32_t const &b)
	{
		uint32_t tmp2 = ~b;
		uint32_t tmp =
			((a & tmp2) + (((a ^ tmp2) >> 1) & 0x7f7f7f7f)) & 0x80808080;

		return ((tmp >> 7) + 0x7f7f7f7f) ^ 0x7f7f7f7f;
	}

	inline uint32_t packed_bytes_add(
		uint32_t const &a,
		uint32_t const &b)
	{
		return a + b - ((((a & b) << 1) + ((a ^ b) & 0xfefefefe)) & 0x01010100);
	}

	inline uint32_t med(
		uint32_t const &a,
		uint32_t const &b,
		uint32_t const &c,
		uint32_t const &v)
	{
		uint32_t aa_gt_bb = make_gt_mask(a, b);
		uint32_t a_xor_b_and_aa_gt_bb = ((a ^ b) & aa_gt_bb);
		uint32_t aa = a_xor_b_and_aa_gt_bb ^ a;
		uint32_t bb = a_xor_b_and_aa_gt_bb ^ b;
		uint32_t n = make_gt_mask(c, bb);
		uint32_t nn = make_gt_mask(aa, c);
		uint32_t m = ~(n | nn);
		return packed_bytes_add((n & aa) | (nn & bb) | ((bb & m) - (c & m) + (aa & m)), v);
	}

	inline uint32_t avg(
		uint32_t const &a,
		uint32_t const &b,
		uint32_t const &c,
		uint32_t const &v)
	{
		return packed_bytes_add((a & b)
			+ (((a ^ b) & 0xfefefefe) >> 1)
			+ ((a ^ b) & 0x01010101), v);
	}

	/* !! */

	#define DO_CHROMA_DECODE_PROTO(B, G, R, A) \
		do \
		{ \
			uint32_t u = *prev_line; \
			p = med(p, u, up, \
				(0xff0000 & ((B) << 16)) + (0xff00 & ((G) << 8)) + (0xff & (R)) + ((A) << 24)); \
			up = u; \
			*current_line = p; \
			current_line ++; \
			prev_line ++; \
			in += step; \
		} \
		while (-- w);

	#define DO_CHROMA_DECODE_PROTO2(B, G, R, A) \
		do \
		{ \
			uint32_t u = *prev_line; \
			p = avg(p, u, up, \
				(0xff0000 & ((B) << 16)) + (0xff00 & ((G) << 8)) + (0xff & (R)) + ((A) << 24)); \
			up = u; \
			*current_line = p; \
			current_line ++; \
			prev_line ++; \
			in += step; \
		} \
		while (-- w);

	#define DO_CHROMA_DECODE(N, R, G, B) \
		case (N<<1): \
			DO_CHROMA_DECODE_PROTO(R, G, B, IA) \
			break; \
		case (N<<1)+1: \
			DO_CHROMA_DECODE_PROTO2(R, G, B, IA) \
			break;

	/* /!! */

	void init_table()
	{
		short golomb_compression_table[golomb_n_count][9] =
		{
			{3, 7, 15, 27, 63, 108, 223, 448, 130, },
			{3, 5, 13, 24, 51, 95, 192, 384, 257, },
			{2, 5, 12, 21, 39, 86, 155, 320, 384, },
			{2, 3, 9, 18, 33, 61, 129, 258, 511, },
		};

		for (int i = 0; i < leading_zero_table_size; i ++)
		{
			int cnt = 0;
			int j;

			for (j = 1; j != leading_zero_table_size && !(i & j);
				j <<= 1, cnt ++);

			cnt ++;

			if (j == leading_zero_table_size)
				cnt = 0;

			leading_zero_table[i] = cnt;
		}

		for (int n = 0; n < golomb_n_count; n ++)
		{
			int a = 0;
			for (int i = 0; i < 9; i ++)
			{
				for (int j = 0; j < golomb_compression_table[n][i]; j ++)
				{
					golomb_bit_length_table[a ++][n] = i;
				}
			}
		}
	}

	void TLG6DecodeGolombValues(uint8_t *pixel_buf, int pixel_count, uint8_t *bit_pool, int channel)
	{
		int n = golomb_n_count - 1;
		int a = 0;

		int bit_pos = 1;
		uint8_t zero = (*bit_pool & 1) ? 0 : 1;
		uint8_t *limit = pixel_buf + pixel_count * 4;

		while (pixel_buf < limit)
		{
			int count;
			{
				uint32_t t = *(uint32_t*)(bit_pool) >> bit_pos;
				int b = leading_zero_table[t & (leading_zero_table_size - 1)];
				int bit_count = b;
				while (!b)
				{
					bit_count += leading_zero_table_bits;
					bit_pos += leading_zero_table_bits;
					bit_pool += bit_pos >> 3;
					bit_pos &= 7;
					t = *(uint32_t*)(bit_pool) >> bit_pos;
					b = leading_zero_table[t & (leading_zero_table_size - 1)];
					bit_count += b;
				}

				bit_pos += b;
				bit_pool += bit_pos >> 3;
				bit_pos &= 7;

				bit_count --;
				count = 1 << bit_count;
				count += ((*(uint32_t*)(bit_pool) >> (bit_pos)) & (count - 1));

				bit_pos += bit_count;
				bit_pool += bit_pos >> 3;
				bit_pos &= 7;
			}

			if (zero)
			{
				do
				{
					*pixel_buf = 0;
					pixel_buf += 4;
				}
				while (-- count);

				zero ^= 1;
			}
			else
			{
				do
				{
					int bit_count;
					int b;

					uint32_t t = *(uint32_t*)(bit_pool) >> bit_pos;
					if (t)
					{
						b = leading_zero_table[t & (leading_zero_table_size - 1)];
						bit_count = b;
						while (!b)
						{
							bit_count += leading_zero_table_bits;
							bit_pos += leading_zero_table_bits;
							bit_pool += bit_pos >> 3;
							bit_pos &= 7;
							t = *(uint32_t*)(bit_pool) >> bit_pos;
							b = leading_zero_table[t & (leading_zero_table_size - 1)];
							bit_count += b;
						}
						bit_count --;
					}
					else
					{
						bit_pool += 5;
						bit_count = bit_pool[-1];
						bit_pos = 0;
						t = *(uint32_t*)(bit_pool);
						b = 0;
					}

					int k = golomb_bit_length_table[a][n];
					int v = (bit_count << k) + ((t >> b) & ((1 << k) - 1));
					int sign = (v & 1) - 1;
					v >>= 1;
					a += v;

					*(uint8_t*)pixel_buf = ((v ^ sign) + sign + 1);
					pixel_buf += 4;

					bit_pos += b;
					bit_pos += k;
					bit_pool += bit_pos >> 3;
					bit_pos &= 7;

					if (-- n < 0)
					{
						a >>= 1;
						n = golomb_n_count - 1;
					}
				}
				while (-- count);

				zero ^= 1;
			}
		}
	}

	void decode_line(
		uint32_t *prev_line,
		uint32_t *current_line,
		int width,
		int start_block,
		int block_limit,
		uint8_t *filtertypes,
		int skip_block_bytes,
		uint32_t *in,
		uint32_t initialp,
		int odd_skip,
		int dir)
	{
		uint32_t p, up;
		int step, i;

		if (start_block)
		{
			prev_line += start_block * w_block_size;
			current_line += start_block * w_block_size;
			p = current_line[-1];
			up = prev_line[-1];
		}
		else
		{
			p = up = initialp;
		}

		in += skip_block_bytes * start_block;
		step = (dir & 1) ? 1 : -1;

		for (i = start_block; i < block_limit; i ++)
		{
			int w = width - i * w_block_size;
			if (w > w_block_size)
				w = w_block_size;

			int ww = w;
			if (step == -1)
				in += ww - 1;

			if (i & 1)
				in += odd_skip * ww;

			switch (filtertypes[i])
			{
				#define IA (uint8_t)((*in>>24)&0xff)
				#define IR (uint8_t)((*in>>16)&0xff)
				#define IG (uint8_t)((*in>>8)&0xff)
				#define IB (uint8_t)((*in)&0xff)
				DO_CHROMA_DECODE( 0, IB, IG, IR);
				DO_CHROMA_DECODE( 1, IB+IG, IG, IR+IG);
				DO_CHROMA_DECODE( 2, IB, IG+IB, IR+IB+IG);
				DO_CHROMA_DECODE( 3, IB+IR+IG, IG+IR, IR);
				DO_CHROMA_DECODE( 4, IB+IR, IG+IB+IR, IR+IB+IR+IG);
				DO_CHROMA_DECODE( 5, IB+IR, IG+IB+IR, IR);
				DO_CHROMA_DECODE( 6, IB+IG, IG, IR);
				DO_CHROMA_DECODE( 7, IB, IG+IB, IR);
				DO_CHROMA_DECODE( 8, IB, IG, IR+IG);
				DO_CHROMA_DECODE( 9, IB+IG+IR+IB, IG+IR+IB, IR+IB);
				DO_CHROMA_DECODE(10, IB+IR, IG+IR, IR);
				DO_CHROMA_DECODE(11, IB, IG+IB, IR+IB);
				DO_CHROMA_DECODE(12, IB, IG+IR+IB, IR+IB);
				DO_CHROMA_DECODE(13, IB+IG, IG+IR+IB+IG, IR+IB+IG);
				DO_CHROMA_DECODE(14, IB+IG+IR, IG+IR, IR+IB+IG+IR);
				DO_CHROMA_DECODE(15, IB, IG+(IB<<1), IR+(IB<<1));

				default: return;
			}

			if (step == 1)
				in += skip_block_bytes - ww;
			else
				in += skip_block_bytes + 1;

			if (i&1)
				in -= odd_skip * ww;

			#undef IR
			#undef IG
			#undef IB
		}
	}

	/* ! */
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

	Image image;
	image.width = header.image_width;
	image.height = header.image_height;
	image.pixels = new uint32_t[image.width * image.height];

	uint32_t *pixel_buf = new uint32_t[4 * header.image_width * h_block_size];
	uint32_t *zeroline = new uint32_t[header.image_width];
	uint32_t *prev_line = zeroline;
	for (uint32_t i = 0; i < header.image_width; i ++)
		zeroline[i] = 0;

	/* ! */
	init_table();
	uint32_t main_count = header.image_width / w_block_size;
	int fraction = header.image_width - main_count * w_block_size;
	for (uint32_t y = 0; y < header.image_height; y += h_block_size)
	{
		uint32_t ylim = y + h_block_size;
		if (ylim >= header.image_height)
			ylim = header.image_height;

		int pixel_count = (ylim - y) * header.image_width;
		for (int c = 0; c < header.channel_count; c ++)
		{
			uint32_t bit_length;
			ifs.read((char*) &bit_length, 4);

			int method = (bit_length >> 30) & 3;
			bit_length &= 0x3fffffff;

			int byte_length = bit_length / 8;
			if (bit_length % 8)
				byte_length ++;

			uint8_t *bit_pool = new uint8_t[byte_length];
			ifs.read((char*) bit_pool, byte_length);

			if (method == 0)
			{
				TLG6DecodeGolombValues((uint8_t*)pixel_buf + c, pixel_count, bit_pool, c);
			}
			else
			{
				throw std::runtime_error("Unsupported encoding method");
			}
		}

		uint8_t *ft = filter_types.data.get() + (y / h_block_size) * header.x_block_count;
		int skip_bytes = (ylim - y) * w_block_size;

		for (uint32_t yy = y; yy < ylim; yy ++)
		{
			uint32_t *current_line = &image.pixels[yy * image.width];

			int dir = (yy&1)^1;
			int odd_skip = ((ylim - yy -1) - (yy - y));

			if (main_count)
			{
				int start = ((header.image_width < w_block_size)
					? header.image_width
					: w_block_size) * (yy - y);

				decode_line(
					prev_line,
					current_line,
					header.image_width,
					0,
					main_count,
					ft,
					skip_bytes,
					pixel_buf + start,
					header.channel_count == 3
						? 0xff000000
						: 0,
					odd_skip,
					dir);
			}

			if (main_count != header.x_block_count)
			{
				int ww = fraction;
				if (ww > w_block_size)
					ww = w_block_size;

				int start = ww * (yy - y);
				decode_line(
					prev_line,
					current_line,
					header.image_width,
					main_count,
					header.x_block_count,
					ft,
					skip_bytes,
					pixel_buf + start,
					header.channel_count == 3
						? 0xff000000
						: 0,
					odd_skip,
					dir);
			}

			prev_line = current_line;
		}
	}

	/* ! */

	return image;
}
