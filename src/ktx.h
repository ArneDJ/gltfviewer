struct header {
	unsigned char identifier[12];
	unsigned int endianness;
	unsigned int gltype;
	unsigned int gltypesize;
	unsigned int glformat;
	unsigned int glinternalformat;
	unsigned int glbaseinternalformat;
	unsigned int pixelwidth;
	unsigned int pixelheight;
	unsigned int pixeldepth;
	unsigned int arrayelements;
	unsigned int faces;
	unsigned int miplevels;
	unsigned int keypairbytes;
};

union keyvaluepair {
	unsigned int size;
	unsigned char rawbytes[4];
};

GLuint load_KTX_cubemap(const char *fpath);

