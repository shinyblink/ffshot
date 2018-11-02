// ┳━┓┳━┓┓━┓┳ ┳┏━┓┏┓┓
// ┣━ ┣━ ┗━┓┃━┫┃ ┃ ┃
// ┇  ┇  ━━┛┇ ┻┛━┛ ┇
// Usage: ffshot | <farbfeld sink>
// Made by vifino. ISC (C) vifino 2018

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <xcb/xcb.h>
//#include <xcb/xcb_aux.h>
#include <xcb/shm.h>
#include <xcb/xcb_image.h>
//#include <sys/ipc.h>
//#include <sys/shm.h>
#include <errno.h>
#include <err.h>

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

static xcb_connection_t* con;
static xcb_screen_t* scr;
static uint32_t win;

static uint16_t pos_x, pos_y;
static uint16_t width, height;
static unsigned char buf[9];

static xcb_get_geometry_cookie_t gc;
static xcb_get_geometry_reply_t* gr;
static xcb_get_image_cookie_t ic;
static xcb_get_image_reply_t* ir;
#if 0
static xcb_shm_query_version_reply_t* shmr;
static xcb_shm_segment_info_t shmi;
#endif

int main(int argc, char* argv[]) {
	if (!(argc == 1 || argc == 2)) { // one arg max
		printf("Usage: %s [wid]\n", argv[0]);
		return 1;
	}

	if (argc == 2) {
		win = (uint32_t) strtoumax(argv[1], (char* *) NULL, 0);
		fprintf(stderr, "win %u\n", win);
	}

	con = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(con))
		errx(2, "Unable to connect to the X server");

	scr = xcb_setup_roots_iterator(xcb_get_setup(con)).data;
	if (!scr)
		errx(2, "Unable to get screen data.");

	if (argc == 1)
		win = scr->root;

	if (!win) {
		fprintf(stderr, "Invalid window number given.\n");
		return 1;
	}

	// Get window geometry.
	gc = xcb_get_geometry(con, win);
	gr = xcb_get_geometry_reply(con, gc, NULL);
	if (!gr)
		errx(1, "0x%08x: no such window");

	pos_x = gr->x;
	pos_y = gr->y;
	width = gr->width;
	height = gr->height;
	free(gr);

	// Get image from the X server. Yuck.
#if 0
	shmr = xcb_shm_query_version_reply(con, xcb_shm_query_version(con), NULL);
	if(!shmr || !shmr->shared_pixmaps)
		errx(2, "SHM error");

	shmi.shmid = shmget(IPC_PRIVATE, width*height*4, IPC_CREAT | 0600);
	shmi.shmaddr = shmat(shmi.shmid, 0, 0);
	shmi.shmseg = xcb_generate_id(con);
	xcb_shm_attach(con, shmi.shmseg, shmi.shmid, ~0);

	ic = xcb_shm_get_image(con, win, pos_x, pos_y, width, height, ~0, XCB_IMAGE_FORMAT_Z_PIXMAP, shmi.shmseg, 0);
	ir = xcb_shm_get_image_reply(con, ic, NULL);
	if (!ir)
		errx(2, "Failed to get Image");
	unsigned char* data = shmi.shmaddr;
#else
	fprintf(stderr, "%08x: %ux%u to %ux%u\n", win, pos_x, pos_y, width, height);
	ic = xcb_get_image(con, XCB_IMAGE_FORMAT_Z_PIXMAP, win, pos_x, pos_y, width, height, ~0);
	ir = xcb_get_image_reply(con, ic, NULL);
	if (!ir)
		errx(2, "Failed to get Image");
	unsigned char* data = xcb_get_image_data(ir);
#endif
	if (!data)
		errx(2, "Failed to get Image data");
	uint32_t bpp = ir->depth;

	// allocate buffer.
	uint16_t* img = malloc(width * height * 8);
	if (!img)
		errx(2, "Failed to allocate buffer.");

	// Output image header
	bwrite(&("farbfeld"), 8);
	*(uint32_t*)buf = htobe32(width);
	*(uint32_t*)(buf + 4) = htobe32(height);
	bwrite(buf, 8);

	unsigned int hasa = 1;

	switch (bpp) {
	case 24:
		hasa = 0;
	case 32:
		break;
	default:
		errx(2, "No support for bit depths other than 24/32 bit: bit depth %i. Fix me?", bpp);
	}

	unsigned int end = width * height;
	unsigned short r, g, b;
	uint32_t i;
	for (i=0; i < end; i++) {
		// write out pixel
		size_t p = i * 4;
		// BGRA? thanks Xorg.
		r = data[p + 2] << 8;
		g = data[p + 1] << 8;
		b = data[p + 0] << 8;

		img[p + 0] = htobe16(r);
		img[p + 1] = htobe16(g);
		img[p + 2] = htobe16(b);
		img[p + 3] = hasa ? htobe16(data[p + 0] * 2) : 0xFFFF;
	}

	bwrite((unsigned char*) img, width * height * 8);

	free(img);
	free(ir);

	/*
	shmctl(shmi.shmid, IPC_RMID, 0);
	xcb_shm_detach(con, shmi.shmseg);
	shmdt(shmi.shmaddr);
	*/

	xcb_disconnect(con);
	return 0;
}
