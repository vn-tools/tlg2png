require 'stringio'

class Tlg5Reader
  MAGIC = "\x54\x4c\x47\x35\x2e\x30\x00\x72\x61\x77\x1a"

  attr_accessor :width
  attr_accessor :height
  attr_accessor :pixels

  def self.read(input_path)
    self.new(input_path)
  end

  private

  def initialize(input_path)
    open(input_path, 'rb') do |file|
      assert_magic(file)
      @header = Tlg5Header.new(file)
      unless [3, 4].include?(@header.channel_count)
        fail 'Unsupported channel count: '  + @header.channel_count.to_s
      end
      skip_block_information(file)
      @width = @header.image_width
      @height = @header.image_height
      @pixels = read_pixels(file)
    end
  end

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

    (block_y..max_y-1).each do |y|
      prev_red = 0
      prev_green = 0
      prev_blue = 0
      prev_alpha = 0

      block_y_shift = (y - block_y) * @header.image_width
      y_shift = y * @header.image_width
      prev_y_shift = (y - 1) * @header.image_width

      (0..@header.image_width - 1).each do |x|
        red = channel_data[2][block_y_shift + x]
        green = channel_data[1][block_y_shift + x]
        blue = channel_data[0][block_y_shift + x]
        alpha = @header.channel_count == 4 ? channel_data[3][block_y_shift + x] : 0

        red += green
        blue += green

        prev_red += red
        prev_green += green
        prev_blue += blue
        prev_alpha += alpha

        output_pixel = [
          prev_red,
          prev_green,
          prev_blue,
          prev_alpha
        ]

        if y > 0
          output_pixel[0] += output_pixels[prev_y_shift + x][0]
          output_pixel[1] += output_pixels[prev_y_shift + x][1]
          output_pixel[2] += output_pixels[prev_y_shift + x][2]
          output_pixel[3] += output_pixels[prev_y_shift + x][3]
        end

        output_pixel[3] = 255 if @header.channel_count == 3
        output_pixels[y_shift + x] = output_pixel
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
    input_data = StringIO.new(block_info.block_data.pack('C*'))
    output_data = []
    flags = 0
    while ! input_data.eof?
      flags >>= 1
      if (flags & 0x100) != 0x100
        flags = input_data.read(1).unpack('C')[0] | 0xff00
      end

      if (flags & 1) == 1
        x0, x1 = input_data.read(2).unpack('CC')
        position = x0 | ((x1 & 0xf) << 8)
        length = 3 + ((x1 & 0xf0) >> 4)
        if length == 18
          length += input_data.read(1).unpack('C')[0]
        end
        (1..length).each do
          c = @compressor_state.text[position]
          output_data.push(c)
          @compressor_state.text[@compressor_state.offset] = c
          @compressor_state.offset += 1
          @compressor_state.offset &= 0xfff
          position += 1
          position &= 0xfff
        end
      else
        c = input_data.read(1).unpack('C')[0]
        output_data.push(c)
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
