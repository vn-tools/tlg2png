require_relative 'image'
require_relative 'tlg_reader'
require 'stringio'

class Tlg5Reader < TlgReader
  MAGIC = "\x54\x4c\x47\x35\x2e\x30\x00\x72\x61\x77\x1a"

  def read_stream(file)
    assert_magic(file)
    @header = Tlg5Header.new(file)
    unless [3, 4].include?(@header.channel_count)
      fail 'Unsupported channel count: '  + @header.channel_count.to_s
    end
    skip_block_information(file)

    image = Image.new
    image.width = @header.image_width
    image.height = @header.image_height
    image.pixels = read_pixels(file)
    image
  end

  private

  def assert_magic(file)
    fail 'Not a TLG5 file ' if file.read(11) != MAGIC
  end

  def skip_block_information(file)
    block_count = (@header.image_height - 1) / @header.block_height + 1
    file.seek(4 * block_count, IO::SEEK_CUR)
  end

  def read_pixels(file)
    @compressor_state = Tlg5CompressorState.new
    pixels = []
    (0..@header.image_height - 1).step(@header.block_height) do |block_y|
      channel_data = read_channel_data(file)
      load_pixel_block_row(channel_data, block_y, pixels)
    end
    pixels
  end

  def load_pixel_block_row(channel_data, block_y, output_pixels)
    max_y = block_y + @header.block_height
    max_y = @header.image_height if max_y > @header.image_height
    use_alpha = @header.channel_count == 4

    (block_y..(max_y - 1)).each do |y|
      prev_red = 0
      prev_green = 0
      prev_blue = 0
      prev_alpha = 0

      block_y_shift = (y - block_y) * @header.image_width
      prev_y_shift = (y - 1) * @header.image_width

      (0..@header.image_width - 1).each do |x|
        red = channel_data[2][block_y_shift + x]
        green = channel_data[1][block_y_shift + x]
        blue = channel_data[0][block_y_shift + x]
        alpha = use_alpha ? channel_data[3][block_y_shift + x] : 0

        red += green
        blue += green

        prev_red += red
        prev_green += green
        prev_blue += blue
        prev_alpha += alpha

        output_red = prev_red
        output_green = prev_green
        output_blue = prev_blue
        output_alpha = prev_alpha

        if y > 0
          index = (prev_y_shift + x) * 4
          output_red += output_pixels[index + 0]
          output_green += output_pixels[index + 1]
          output_blue += output_pixels[index + 2]
          output_alpha += output_pixels[index + 3]
        end

        output_alpha = 255 unless use_alpha

        output_pixels << output_red
        output_pixels << output_green
        output_pixels << output_blue
        output_pixels << output_alpha
      end
    end
  end

  def read_channel_data(file)
    channel_data = {}
    (0..@header.channel_count - 1).each do |channel|
      block_info = Tlg5BlockInfo.new(file)
      decompress_block(block_info) unless block_info.mark
      channel_data[channel] = block_info.block_data
    end
    channel_data
  end

  def decompress_block(block_info)
    input_data = block_info.block_data
    output_data = []
    flags = 0
    i = 0
    while i < block_info.block_size
      flags >>= 1
      if (flags & 0x100) != 0x100
        flags = input_data[i] | 0xff00
        i += 1
      end

      if (flags & 1) == 1
        x0, x1 = input_data[i], input_data[i + 1]
        i += 2
        position = x0 | ((x1 & 0xf) << 8)
        length = 3 + ((x1 & 0xf0) >> 4)
        if length == 18
          length += input_data[i]
          i += 1
        end
        j = 0
        while j < length
          c = @compressor_state.text[position]
          output_data << c
          @compressor_state.text[@compressor_state.offset] = c
          @compressor_state.offset += 1
          @compressor_state.offset &= 0xfff
          position += 1
          position &= 0xfff
          j += 1
        end
      else
        c = input_data[i]
        i += 1
        output_data << c
        @compressor_state.text[@compressor_state.offset] = c
        @compressor_state.offset += 1
        @compressor_state.offset &= 0xfff
      end
    end
    block_info.block_data = output_data
  end

  class Tlg5BlockInfo
    attr_reader :mark
    attr_reader :block_size
    attr_accessor :block_data

    def initialize(file)
      @mark = file.read(1).unpack('C')[0] > 0
      @block_size = file.read(4).unpack('<L')[0]
      @block_data = file.read(@block_size).unpack('C*')
    end
  end

  class Tlg5Header
    attr_reader :channel_count
    attr_reader :image_width
    attr_reader :image_height
    attr_reader :block_height

    def initialize(file)
      @channel_count = file.read(1).unpack('C')[0]
      @image_width = file.read(4).unpack('<L')[0]
      @image_height = file.read(4).unpack('<L')[0]
      @block_height = file.read(4).unpack('<L')[0]
    end
  end

  class Tlg5CompressorState
    attr_reader :text
    attr_accessor :offset

    def initialize
      @text = Array(0..4095).fill(0)
      @offset = 0
    end
  end
end
