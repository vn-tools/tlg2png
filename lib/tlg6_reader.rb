class Tlg6Reader
  MAGIC = "\x54\x4c\x47\x36\x2e\x30\x00\x72\x61\x77\x1a"

  attr_accessor :width
  attr_accessor :height
  attr_accessor :pixels

  def self.read(_input_path)
    fail 'Not implemented! Please send sample files to @rr- on github.'
  end
end
