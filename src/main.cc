#include <iostream>
#include <stdexcept>
#include <string>
#include "TlgConverter.h"

void show_usage(std::string path_to_self)
{
	std::cerr << "Usage: " << path_to_self << " INPUT OUTPUT" << std::endl;
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		show_usage(argv[0]);
		return 1;
	}

	std::string input_path(argv[1]);
	std::string output_path(argv[2]);

	try
	{
		TlgConverter converter;
		auto image = converter.read(input_path);
		converter.save(image, output_path);
	}
	catch (std::exception const &e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
