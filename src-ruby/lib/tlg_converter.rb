require_relative 'tlg5_reader'
require_relative 'tlg6_reader'
require 'rubygems'
require 'RMagick'

class TlgConverter
  def self.read(input_path)
    self.new(input_path)
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
      @reader = reader(file).read(file.path)
    end
  end

  def reader(file)
    magic = file.read(11)
    return Tlg5Reader if magic == Tlg5Reader::MAGIC
    return Tlg6Reader if magic == Tlg6Reader::MAGIC
    fail 'Not a TLG image file'
  end
end
