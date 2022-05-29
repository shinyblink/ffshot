// ┳━┓┳━┓┓━┓┳ ┳┏━┓┏┓┓
// ┣━ ┣━ ┗━┓┃━┫┃ ┃ ┃
// ┇  ┇  ━━┛┇ ┻┛━┛ ┇
// Usage: ffshot | <farbfeld sink>
// Made by vifino. ISC (C) vifino 2018

#include <err.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb_image.h>

// I know, I know, not standardized.
// But painless fallback.
#if defined(__linux__)
#include <endian.h>
#if __BYTE_ORDER != __BIG_ENDIAN
#define DOCONVERT
#endif
#else
#define DOCONVERT
#include <arpa/inet.h>
#define be32toh ntohl
#define htobe32 htonl
#define be16toh ntohs
#define htobe16 htons
#endif

static inline void bwrite(const unsigned char* buffer, size_t bytes) {
	if (!fwrite(buffer, bytes, 1, stdout)) {
		fprintf(stderr, "write failed.\n");
		exit(1);
	}
}

static uint32_t wid;
static uint16_t width, height;
static unsigned char buf[9];

static xcb_connection_t* con;
static xcb_get_geometry_cookie_t gc;
static xcb_get_geometry_reply_t* gr;
static xcb_get_image_cookie_t ic;
static xcb_get_image_reply_t* ir;

int main(int argc, char* argv[]) {
	if (argc > 2) { // one arg max
		printf("Usage: %s [wid]\n", argv[0]);
		return 1;
	}

	con = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(con))
		errx(2, "Unable to connect to the X server");

	if (argc == 2)
		wid = (uint32_t) strtoumax(argv[1], (char* *) NULL, 0);
	else
		wid=xcb_setup_roots_iterator(xcb_get_setup(con)).data->root;

	if (!wid)
		errx(1, "Invalid window number given.\n");

	// Get window geometry.
	gc = xcb_get_geometry(con, wid);
	gr = xcb_get_geometry_reply(con, gc, NULL);
	if (!gr)
		errx(1, "0x%08x: no such window", wid);

	width = gr->width;
	height = gr->height;
	free(gr);

	// Get image from the X server. Yuck.
	ic = xcb_get_image(con, XCB_IMAGE_FORMAT_Z_PIXMAP, wid, 0, 0, width, height, ~0);
	ir = xcb_get_image_reply(con, ic, NULL);
	if (!ir)
		errx(2, "Failed to get Image");

	uint32_t bpp = ir->depth;
	uint32_t* data = (uint32_t*) xcb_get_image_data(ir);
	if (!data)
		errx(2, "Failed to get Image data");

	unsigned int hasa = 1;
	unsigned int bpc = 8;
	switch (bpp) {
	case 24:
		hasa = 0;
		break;
	case 30:
		bpc = 10;
		hasa = 0;
	case 32:
		break;
	default:
		errx(2, "No support for bit depths other than 24/30/32 bit: bit depth %i. Fix me?", bpp);
	}
	
	fprintf(stderr, "%08x: %ux%u @ %ubpp\n", wid, width, height, bpp);

	// allocate buffer.
	uint16_t* img = malloc(width * height * 8);
	if (!img)
		errx(2, "Failed to allocate buffer.");

	// Output image header
	bwrite((unsigned char*)("farbfeld"), 8);
	*(uint32_t*)buf = htobe32(width);
	*(uint32_t*)(buf + 4) = htobe32(height);
	bwrite(buf, 8);

	size_t end = width * height;
	uint16_t r, g, b, a;
	uint32_t i, p;
	uint32_t mask = (2 << (bpc - 1)) - 1;
	uint32_t scale = (1 << (16 - bpc)) + 1;
	for (i=0; i < end; i++) {
		// write out pixel
		// BGRA? thanks Xorg.
		b = (data[i] >> (0*bpc)) & mask;
		g = (data[i] >> (1*bpc)) & mask;
		r = (data[i] >> (2*bpc)) & mask;
		a = (data[i] >> (3*bpc)) & mask;

		p = i * 4;
		img[p + 0] = htobe16(r * scale);
		img[p + 1] = htobe16(g * scale);
		img[p + 2] = htobe16(b * scale);
		img[p + 3] = hasa ? htobe16(a * scale) : 0xFFFF;
	}

	bwrite((unsigned char*) img, width * height * 8);

	free(img);
	free(ir);
	xcb_disconnect(con);
	return 0;
}
