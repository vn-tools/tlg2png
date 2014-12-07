require_relative 'tlg_reader'
require_relative 'tlg5_reader'
require_relative 'tlg6_reader'
require 'rubygems'
require 'RMagick'

class Tlg0Reader < TlgReader
  MAGIC = "\x54\x4c\x47\x30\x2e\x30\x00\x73\x64\x73\x1a"

  def read_stream(file)
    assert_magic(file)
    _raw_data_size = file.read(4).unpack('<L')[0]
    magic2 = file.read(11)
    file.seek(-11, 1)
    image = reader_from_magic(magic2).new.read_stream(file)
    process_chunks(file)
    image
  end

  private

  def process_chunks(file)
    until file.eof?
      chunk_name = file.read(4)
      chunk_size = file.read(4).unpack('<L')[0]
      chunk_data = file.read(chunk_size)
      process_tag_chunk(chunk_data) if chunk_name == 'tags'
    end
  end

  def process_tag_chunk(chunk_data)
    until chunk_data.empty?
      key, chunk_data = extract_string(chunk_data)
      _, _, chunk_data = chunk_data.partition('=')
      value, chunk_data = extract_string(chunk_data)
      _, _, chunk_data = chunk_data.partition(',')
      puts 'Tag found: ' + key + ' = ' + value
    end
  end

  def extract_string(raw)
    size, _colon, raw = raw.partition(':')
    size = size.to_i
    value = raw[0..(size - 1)]
    raw = raw[size..-1]
    [value, raw]
  end

  def assert_magic(file)
    fail 'Not a TLG0 file ' if file.read(11) != MAGIC
  end

  def reader_from_magic(magic)
    return Tlg5Reader if magic == Tlg5Reader::MAGIC
    return Tlg6Reader if magic == Tlg6Reader::MAGIC
  end
end
