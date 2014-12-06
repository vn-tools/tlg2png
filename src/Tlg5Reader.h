#ifndef TLG5_READER_H_
#define TLG5_READER_H_
#include "AbstractTlgReader.h"

class Tlg5Reader : public AbstractTlgReader
{
	public:
		virtual const std::string get_magic() const;

	protected:
		virtual const Image read_from_stream(std::ifstream &stream) const;
};

#endif
