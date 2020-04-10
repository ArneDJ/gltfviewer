enum {
	FOURCC_DXT1 = 0x31545844, 
	FOURCC_DXT3 = 0x33545844, 
	FOURCC_DXT5 = 0x35545844,
};

enum {
	DDSF_CUBEMAP = 0x00000200,
	DDSF_CUBEMAP_POSITIVEX = 0x00000400,
	DDSF_CUBEMAP_NEGATIVEX = 0x00000800,
	DDSF_CUBEMAP_POSITIVEY = 0x00001000,
	DDSF_CUBEMAP_NEGATIVEY = 0x00002000,
	DDSF_CUBEMAP_POSITIVEZ = 0x00004000,
	DDSF_CUBEMAP_NEGATIVEZ = 0x00008000,
	DDSF_CUBEMAP_ALL_FACES = 0x0000FC00,
};

// DDS header
struct DDS {
	char identifier[4]; /* file type */
	uint32_t height; /* in pixels */
	uint32_t width; /* in pixels */
	uint32_t linear_size;
	uint32_t mip_levels;
	uint32_t dxt_codec; /* compression type */
	uint32_t dwcaps2;
};

// cubemaps importing isn't supported
unsigned char *load_DDS(const char *fpath, struct DDS *header);
