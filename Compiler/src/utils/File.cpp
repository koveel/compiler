#include "pch.h"
#include "File.h"

#include <fstream>

File read_file_contents(const std::filesystem::path& path)
{
	std::ifstream in(path, std::ios::in | std::ios::binary);
	ASSERT(in && "could not open file");

	in.seekg(0, std::ios::end);
	std::streampos size = in.tellg();
	ASSERT(size != -1 && "could not read file");

	File file = { new char[(size_t)size + 1], size };

	in.seekg(0, std::ios::beg);
	in.read(file.data, size);
	in.close();

	file.data[size] = 0;
	return file;
}