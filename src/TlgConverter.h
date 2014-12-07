#ifndef TLG_CONVERTER_H_
#define TLG_CONVERTER_H_
#include <string>
#include "Image.h"

class TlgConverter
{
	public:
		const Image read(std::string path) const;
		void save(const Image image, std::string path) const;
};

#endif
