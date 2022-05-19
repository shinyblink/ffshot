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

#define IC xcb_get_image(con, XCB_IMAGE_FORMAT_Z_PIXMAP, wid, 0, 0, width, height, ~0)
#define CD(ch) ((data[i] >> (ch * bpc)) & mask)

static inline void bwrite(const unsigned char* buffer, size_t bytes) {
	if (!fwrite(buffer, bytes, 1, stdout)) {
		fprintf(stderr, "write failed.\n");
		exit(1);
	}
}

static xcb_connection_t* con;
static uint32_t wid;

static uint16_t width, height;
static unsigned char buf[9];

static xcb_get_geometry_reply_t* gr;
static xcb_get_image_reply_t* ir;

int main(int argc, char* argv[]) {
	if ((argc > 2)) { // one arg max
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
	gr = xcb_get_geometry_reply(con, xcb_get_geometry(con, wid), NULL);
	if (!gr)
		errx(1, "0x%08x: no such window", wid);

	width = gr->width;
	height = gr->height;
	free(gr);

	// Get image from the X server. Yuck.
	fprintf(stderr, "%08x: %ux%u\n", wid, width, height);
	ir = xcb_get_image_reply(con, IC, NULL);
	if (!ir)
		errx(2, "Failed to get Image");

	uint32_t* data = (uint32_t*) xcb_get_image_data(ir);
	if (!data)
		errx(2, "Failed to get Image data");

	unsigned int bpc = 8;
	switch (ir->depth) {
	case 24:
	case 32:
		break;
	case 30:
		bpc = 10;
		break;
	default:
		errx(2, "No support for bit depths other than 24/30/32 bit: bit depth %i. Fix me?", ir->depth);
	}

	// allocate buffer.
	uint16_t* img = malloc(width * height * 8);
	if (!img)
		errx(2, "Failed to allocate buffer.");

	// Output image header
	bwrite((unsigned char*)("farbfeld"), 8);
	*(uint32_t*)buf = htobe32(width);
	*(uint32_t*)(buf + 4) = htobe32(height);
	bwrite(buf, 8);

	uint32_t i;
	uint32_t p;
	uint32_t mask = (1 << bpc) - 1;
	for (i = 0; i < width * height; i++) {
		// write out pixel
		// BGRA? thanks Xorg.

		p = i * 4;
		img[p + 0] = htobe16(CD(2) * (mask + 2));                         // r
		img[p + 1] = htobe16(CD(1) * (mask + 2));                         // g
		img[p + 2] = htobe16(CD(0) * (mask + 2));                         // b
		img[p + 3] = ir->depth / 32 ? htobe16(CD(3) * (mask+2)) : 0xFFFF; // a
	}

	bwrite((unsigned char*) img, width * height * 8);

	free(img);
	free(ir);

	xcb_disconnect(con);
	return 0;
}
