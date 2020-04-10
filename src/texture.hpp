struct image_t {
	unsigned char *data;
	unsigned int nchannels;
	size_t width;
	size_t height;
};

GLuint load_texture(const char *fpath);
GLuint load_cubemap_texture(const char *fpath[6]);

GLuint load_DDS_texture(const char *fpath);

GLuint gen_texture(struct image_t *image, GLenum internalformat, GLenum format, GLenum type);
