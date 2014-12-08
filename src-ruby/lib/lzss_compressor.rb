# LZSS-based compressor used by TLG5 and TLG6.
class LzssCompressor
  def self.decompress(compressor_state, input_data, input_size)
    output_data = []
    flags = 0
    i = 0
    while i < input_size
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
          c = compressor_state.text[position]
          output_data << c
          compressor_state.text[compressor_state.offset] = c
          compressor_state.offset += 1
          compressor_state.offset &= 0xfff
          position += 1
          position &= 0xfff
          j += 1
        end
      else
        c = input_data[i]
        i += 1
        output_data << c
        compressor_state.text[compressor_state.offset] = c
        compressor_state.offset += 1
        compressor_state.offset &= 0xfff
      end
    end
    output_data
  end
end
