#pragma once

struct File {
	char*  data = nullptr;
	size_t size = 0ull;
};

File read_file_contents(const std::filesystem::path& path);