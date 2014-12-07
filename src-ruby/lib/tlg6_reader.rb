require_relative 'tlg_reader'

class Tlg6Reader < TlgReader
  MAGIC = "\x54\x4c\x47\x36\x2e\x30\x00\x72\x61\x77\x1a"

  def read_stream(file)
    fail 'Not a TLG6 file ' if file.read(11) != MAGIC
    fail 'Not implemented! Please send sample files to @rr- on github.'
  end
end
