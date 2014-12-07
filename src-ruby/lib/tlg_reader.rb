class TlgReader
  def read(input_path)
    open(input_path, 'rb') { |file| read_stream(file) }
  end

  def read_stream(file)
    fail 'Implement me'
  end
end
