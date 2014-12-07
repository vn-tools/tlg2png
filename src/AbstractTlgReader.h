#ifndef ABSTRACT_TLG_READER_H_
#define ABSTRACT_TLG_READER_H_
#include <fstream>
#include <memory>
#include <string>
#include "Image.h"

class AbstractTlgReader
{
	friend class Tlg0Reader;

	public:
		static std::unique_ptr<const AbstractTlgReader>
			choose_reader(std::ifstream &ifs);

		const Image read(std::string path) const;

	private:
		virtual const Image read_raw_data(std::ifstream &stream) const = 0;

		virtual const std::string get_magic() const = 0;

		const bool is_magic_valid(std::ifstream &ifs) const;
};

#endif
