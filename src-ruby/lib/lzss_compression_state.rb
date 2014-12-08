# Holds TLG compression state.
class LzssCompressionState
  # dictionary used by the modified LZW compression algorithm
  attr_reader :text

  # offset within the dictionary
  attr_accessor :offset

  def initialize
    @text = Array(0..4095).fill(0)
    @offset = 0
  end
end
