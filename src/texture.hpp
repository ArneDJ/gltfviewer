struct image_t {
	unsigned char *data;
	unsigned int nchannels;
	size_t width;
	size_t height;
};

GLuint load_DDS_texture(const char *fpath);
GLuint load_DDS_cubemap(const char *fpath[6]);

GLuint gen_texture(struct image_t *image, GLenum internalformat, GLenum format, GLenum type);
