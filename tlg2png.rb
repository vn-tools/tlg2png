#!/usr/bin/ruby
require_relative 'lib/tlg_converter.rb'
require 'ostruct'
require 'optparse'

# CLI frontend for TLG unpacker
class CLI
  def initialize
    parse_options
  end

  def run
    TlgConverter.read(@options.input_path).save(@options.output_path)
  end

  private

  def parse_options
    @options = OpenStruct.new
    @options.input_path = nil
    @options.output_path = nil

    opt_parser = OptionParser.new do |opts|
      opts.banner = format(
        'Usage: %s INPUT_PATH OUTPUT_PATH',
        File.basename(__FILE__))

      opts.separator ''

      opts.on_tail('-h', '--help', 'Show this message') do
        puts opts
        exit
      end
    end

    begin
      opt_parser.parse!
    rescue OptionParser::InvalidOption => e
      puts e.message
      puts
      puts opt_parser
      exit(1)
    end

    if ARGV.length != 2
      puts opt_parser
      exit(1)
    end

    @options.input_path = ARGV.first
    @options.output_path = ARGV.last
  end

  def escape(string)
    string
      .sub(/^\//, '')
      .gsub(/([^ a-zA-Z0-9_.-]+)/n, '_')
  end
end

CLI.new.run
