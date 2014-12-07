# A base class for all TLG reader implementations.
class TlgReader
  def read(input_path)
    open(input_path, 'rb') { |file| read_stream(file) }
  end

  def read_stream(_file)
    fail 'Implement me in a derived class'
  end
end
