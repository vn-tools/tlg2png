require_relative 'tlg0_reader'
require_relative 'tlg5_reader'
require_relative 'tlg6_reader'
require 'rubygems'
require 'RMagick'

class TlgConverter
  def self.read(input_path)
    new(input_path)
  end

  def save(output_path)
    img = Magick::Image.new(@reader.width, @reader.height)

    img.import_pixels(
      0, 0,
      @reader.width, @reader.height,
      'RGBA',
      @reader.pixels.flatten.pack('C*'),
      Magick::CharPixel)

    img.write(output_path)
  end

  private

  def initialize(input_path)
    open(input_path, 'rb') do |file|
      @reader = reader_type(file).new.read_stream(file)
    end
  end

  def reader_type(file)
    magic = file.read(11)
    file.seek(0, 0)
    return Tlg0Reader if magic == Tlg0Reader::MAGIC
    return Tlg5Reader if magic == Tlg5Reader::MAGIC
    return Tlg6Reader if magic == Tlg6Reader::MAGIC
    fail 'Not a TLG image file'
  end
end
