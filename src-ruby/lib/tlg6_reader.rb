require_relative 'tlg_reader'
require_relative 'lzss_compression_state'

# A TLG reader that handles TLG version 6.
class Tlg6Reader < TlgReader
  MAGIC = "\x54\x4c\x47\x36\x2e\x30\x00\x72\x61\x77\x1a"
  TLG6_W_BLOCK_SIZE = 8
  TLG6_H_BLOCK_SIZE = 8

  def read_stream(file)
    fail 'Not a TLG6 file ' if file.read(11) != MAGIC
    @header = Tlg6Header.new(file)
    unless [1, 3, 4].include?(@header.channel_count)
      fail 'Unsupported channel count: ' + @header.channel_count.to_s
    end

    x_block_count = ((@header.image_width - 1) / TLG6_W_BLOCK_SIZE) + 1;
    y_block_count = ((@header.image_height - 1) / TLG6_H_BLOCK_SIZE) + 1;

    @filter_types = Tlg6FilterTypes.new(file)
    @filter_types.decompress(@header)

    fail 'Not implemented! Please send sample files to @rr- on github.'
  end

  private

  # A header of TLG6 file.
  class Tlg6Header
    attr_reader :channel_count
    attr_reader :data_flag
    attr_reader :color_type
    attr_reader :external_golomb_table
    attr_reader :image_width
    attr_reader :image_height
    attr_reader :max_bit_length

    def initialize(file)
      @channel_count = file.read(1).unpack('C')[0]
      @data_flag = file.read(1).unpack('C')[0]
      @color_type = file.read(1).unpack('C')[0]
      @external_golomb_table = file.read(1).unpack('C')[0]
      @image_width = file.read(4).unpack('<L')[0]
      @image_height = file.read(4).unpack('<L')[0]
      @max_bit_length = file.read(4).unpack('<L')[0]
    end
  end

  # Filter internally used by TLG6.
  class Tlg6FilterTypes
    attr_reader :data_size
    attr_reader :data

    def initialize(file)
      @data_size = file.read(4).unpack('<L')[0]
      @data = file.read(data_size).unpack('C*')
    end

    def decompress(file)
      compression_state = LzssCompressionState.new

      z = 0
      (0..31).each do |i|
        (0..15).each do |j|
          (0..3).each { compression_state.text[z += 1] = i }
          (0..3).each { compression_state.text[z += 1] = j }
        end
      end

      @data = LzssCompressor.decompress(compression_state, @data, @data_size)
    end
  end

end
