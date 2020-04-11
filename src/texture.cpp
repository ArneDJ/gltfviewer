#include <iostream>
#include <GL/glew.h>
#include <GL/gl.h>

#include "dds.hpp"
#include "texture.hpp"

GLuint load_DDS_cubemap(const char *fpath[6])
{
	GLuint texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

	for (int face = 0; face < 6; face++) {
		GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;
		struct DDS header;
		unsigned char *image = load_DDS(fpath[face], &header);
		/* find valid DXT format*/
		uint32_t components = (header.dxt_codec == FOURCC_DXT1) ? 3 : 4;
		uint32_t format;
		uint32_t block_size;
		switch (header.dxt_codec) {
		case FOURCC_DXT1: format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; block_size = 8; break;
		case FOURCC_DXT3: format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; block_size = 16; break;
		case FOURCC_DXT5: format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; block_size = 16; break;
		default:
		std::cerr << "error: no valid DXT format found for " << fpath << std::endl;
		delete [] image;
		return 0;
		};
		unsigned int size = ((header.width+3)/4) * ((header.height+3)/4) * block_size;
		glCompressedTexImage2D(target, 0, format,
		header.width, header.height, 0, size, image);

		delete [] image;
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	return texture;
}

GLuint load_DDS_texture(const char *fpath)
{
	struct DDS header;
	unsigned char *image = load_DDS(fpath, &header);
	if (image == nullptr) { return 0; }

	/* find valid DXT format*/
	uint32_t components = (header.dxt_codec == FOURCC_DXT1) ? 3 : 4;
	uint32_t format;
	uint32_t block_size;
	switch (header.dxt_codec) {
	case FOURCC_DXT1: format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; block_size = 8; break;
	case FOURCC_DXT3: format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; block_size = 16; break;
	case FOURCC_DXT5: format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; block_size = 16; break;
	default:
	std::cerr << "error: no valid DXT format found for " << fpath << std::endl;
	delete [] image;
	return 0;
	};

	/* now make the opengl texture */
	GLuint texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, header.mip_levels-1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	unsigned int offset = 0;
	for (unsigned int i = 0; i < header.mip_levels; i++) {
		if (header.width <= 4 || header.height <= 4) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, i-1);
		break;
		}
		/* now to actually get the compressed image into opengl */
		unsigned int size = ((header.width+3)/4) * ((header.height+3)/4) * block_size;
		glCompressedTexImage2D(GL_TEXTURE_2D, i, format,
		header.width, header.height, 0, size, image + offset);

		offset += size;
		header.width = header.width/2;
		header.height = header.height/2;
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	delete [] image;

	return texture;
}

// generate mip mapped texture
GLuint gen_texture(struct image_t *image, GLenum internalformat, GLenum format, GLenum type)
{
	GLuint texture;

	GLsizei NUM_MIPMAPS = 6;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexStorage2D(GL_TEXTURE_2D, NUM_MIPMAPS, internalformat, image->width, image->height);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image->width, image->height, format, type, image->data);

 	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}
