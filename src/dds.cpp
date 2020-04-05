#include <cstdlib>
#include <cstring>
#include <iostream>

#include "dds.hpp"

unsigned char *load_DDS(const char *fpath, struct DDS *header)
{
	FILE *fp = fopen(fpath, "rb");
	if (fp == nullptr) {
		perror(fpath);
		return nullptr;
	}

	/* verify the type of file */
	fread(header->identifier, 1, 4, fp);
	if (strncmp(header->identifier, "DDS ", 4) != 0) {
		std::cerr << "error: " << fpath << "not a valid DDS file\n";
		fclose(fp);
		return nullptr;
	}

	/* the header is 128 bytes, 124 after reading the file type */
	unsigned char header_buf[124];
	fread(&header_buf, 124, 1, fp);
	header->height = *(uint32_t*)&(header_buf[8]);
	header->width = *(uint32_t*)&(header_buf[12]);
	header->linear_size = *(uint32_t*)&(header_buf[16]);
	header->mip_levels = *(uint32_t*)&(header_buf[24]);
	header->dxt_codec = *(uint32_t*)&(header_buf[80]);

	/* now get the actual image data */
	unsigned char *image;
	uint32_t len = (header->mip_levels > 1) ? (header->linear_size * 2) : header->linear_size;
	image = new unsigned char[len];
	fread(image, 1, len, fp);

	fclose(fp);

	return image;
}
