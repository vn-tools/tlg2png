#ifndef BAD_MAGIC_EXCEPTION_H_
#define BAD_MAGIC_EXCEPTION_H_

class BadMagicException : public std::exception
{
	const char *what() const noexcept
	{
		return "Not a TLG image";
	}
};

#endif
