#pragma once

#include <string>
#include <stdio.h>

using namespace std;

// Various file operations, implemented in standard C

// Copy files from src to dst using stdio
inline void copy_file(const std::string& src, const std::string& dst){
	char buf[8192];
    size_t size;
	FILE* source = fopen(src.c_str(), "rb");
    FILE* dest = fopen(dst.c_str(), "wb");
    while (size = fread(buf, 1, BUFSIZ, source)) {
        fwrite(buf, 1, size, dest);
    }
    fclose(source);
    fclose(dest);
}

// Check if a file exists using stdio
inline bool file_exists(const std::string& name) {
	if (FILE *file = fopen(name.c_str(), "r")) {
		fclose(file);
		return true;
	} else {
		return false;
	}   
}