#!/usr/bin/python
import sys
import struct
from PIL import Image

if len(sys.argv) < 3:
	print >>sys.stderr, 'Usage: ' + sys.argv[0] + ' INPUT OUPTUT'
	sys.exit(1)

input_path = sys.argv[1]
output_path = sys.argv[2]

with open(input_path, 'rb') as input_fh:
	header = input_fh.read(11)
	if header != 'TLG5.0\x00raw\x1a':
		raise Exception('Unsupported file version: ' + header.encode('hex'))

	channels, \
	width, \
	height, \
	block_height = struct.unpack('=bIII', input_fh.read(13))

	if channels != 3 and channels != 4:
		raise Exception('Unsupported channel count: ' + channels)

	block_count = (height - 1) / block_height + 1
	input_fh.seek(block_count * 4, 1)

	pixels = {}
	for x in range(width):
		pixels[x,-1] = (0, 0, 0, 0)
	text = [0 for i in range(4096)]
	outs = [[0] * ((block_height * width + 10 + 3) & ~3) for channel in range(channels)]
	r = 0

	for block_y in xrange(0, height, block_height):
		for channel in xrange(channels):
			mark, block_size = struct.unpack('=?I', input_fh.read(5))
			buf = bytearray(input_fh.read(block_size))

			if mark != 0:
				for i in xrange(len(buf)):
					outs[channel][i] = buf[i]
			else:
				outi = 0
				flags = 0
				i = 0
				while i < block_size:
					flags >>= 1
					if not flags & 0x100:
						flags = buf[i] | 0xff00
						i += 1

					if flags & 1:
						x0 = buf[i]
						x1 = buf[i+1]
						i += 2
						mpos = x0 | ((x1 & 0xf) << 8)
						mlen = (x1 & 0xf0) >> 4
						mlen += 3
						if mlen == 18:
							mlen += buf[i]
							i += 1
						for j in xrange(mlen):
							text[r] = text[mpos]
							outs[channel][outi] = text[r]
							outi += 1
							mpos += 1
							mpos &= 0xfff
							r += 1
							r &= 0xfff
					else:
						c = buf[i]
						i += 1
						outs[channel][outi] = c
						outi += 1
						text[r] = c
						r += 1
						r &= 0xfff

			assert i == block_size

		#swap BGR to RGB
		t = outs[2]
		outs[2] = outs[0]
		outs[0] = t

		max_y = block_y + block_height
		if max_y > height:
			max_y = height

		for y in xrange(block_y, max_y):
			pc = [0] * channels
			oc = [0, 0, 0, 255]
			c = [0] * channels
			row_shift = (y - block_y) * width
			for x in xrange(width):
				for channel in xrange(channels):
					c[channel] = outs[channel][row_shift + x]
				c[0] += c[1]
				c[2] += c[1]
				for channel in xrange(channels):
					pc[channel] += c[channel]
					oc[channel] = (pc[channel] + pixels[x,y-1][channel]) & 0xff
				pixels[x,y] = tuple(oc)

	output_image = Image.new('RGBA', (width, height))
	output_pixels = output_image.load()
	for x in range(width):
		del pixels[x,-1]
	for x,y in pixels.keys():
		output_pixels[x,y] = pixels[x,y]

	output_image.save(output_path)
