#ifndef ABSTRACT_TLG_READER_H_
#define ABSTRACT_TLG_READER_H_
#include <cstdint>
#include <string>
#include "Image.h"

class AbstractTlgReader
{
	public:
		virtual const std::string get_magic() const = 0;
		const Image read(std::string path) const;

	protected:
		virtual const Image read_from_stream(std::ifstream &stream) const = 0;
};

#endif
