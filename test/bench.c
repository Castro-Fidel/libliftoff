#define _POSIX_C_SOURCE 200112L
#include <assert.h>
#include <libliftoff.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "libdrm_mock.h"
#include "log.h"

#define MAX_PLANES 128
#define MAX_LAYERS 128

static struct liftoff_layer *add_layer(struct liftoff_output *output,
				       int x, int y, int width, int height)
{
	uint32_t fb_id;
	struct liftoff_layer *layer;

	layer = liftoff_layer_create(output);
	fb_id = liftoff_mock_drm_create_fb(layer);
	liftoff_layer_set_property(layer, "FB_ID", fb_id);
	liftoff_layer_set_property(layer, "CRTC_X", x);
	liftoff_layer_set_property(layer, "CRTC_Y", y);
	liftoff_layer_set_property(layer, "CRTC_W", width);
	liftoff_layer_set_property(layer, "CRTC_H", height);
	liftoff_layer_set_property(layer, "SRC_X", 0);
	liftoff_layer_set_property(layer, "SRC_Y", 0);
	liftoff_layer_set_property(layer, "SRC_W", width << 16);
	liftoff_layer_set_property(layer, "SRC_H", height << 16);

	return layer;
}

int main(int argc, char *argv[])
{
	int opt;
	size_t planes_len, layers_len;
	struct timespec start, end;
	struct liftoff_mock_plane *mock_planes[MAX_PLANES];
	size_t i, j;
	int plane_type;
	int drm_fd;
	struct liftoff_device *device;
	struct liftoff_output *output;
	struct liftoff_layer *layers[MAX_LAYERS];
	drmModeAtomicReq *req;
	bool ok;

	planes_len = 5;
	layers_len = 10;
	while ((opt = getopt(argc, argv, "p:l:")) != -1) {
		switch (opt) {
		case 'p':
			planes_len = atoi(optarg);
			break;
		case 'l':
			layers_len = atoi(optarg);
			break;
		default:
			fprintf(stderr, "usage: %s [-p planes] [-l layers]\n",
				argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	liftoff_log_set_priority(LIFTOFF_SILENT);

	for (i = 0; i < planes_len; i++) {
		plane_type = i == 0 ? DRM_PLANE_TYPE_PRIMARY :
				      DRM_PLANE_TYPE_OVERLAY;
		mock_planes[i] = liftoff_mock_drm_create_plane(plane_type);
	}

	drm_fd = liftoff_mock_drm_open();
	device = liftoff_device_create(drm_fd);
	assert(device != NULL);

	liftoff_device_register_all_planes(device);

	output = liftoff_output_create(device, liftoff_mock_drm_crtc_id);

	for (i = 0; i < layers_len; i++) {
		/* Planes don't intersect, so the library can arrange them in
		 * any order. Testing all combinations takes more time. */
		layers[i] = add_layer(output, i * 100, i * 100, 100, 100);
		for (j = 0; j < planes_len; j++) {
			if (j == 1) {
				/* Make the lowest plane above the primary plane
				 * incompatible with all layers. A solution
				 * using all planes won't be reached, so the
				 * library will keep trying more combinations.
				 */
				continue;
			}
			liftoff_mock_plane_add_compatible_layer(mock_planes[j],
								layers[i]);
		}
	}

	clock_gettime(CLOCK_MONOTONIC, &start);

	req = drmModeAtomicAlloc();
	ok = liftoff_output_apply(output, req, 0);
	assert(ok);
	drmModeAtomicFree(req);

	clock_gettime(CLOCK_MONOTONIC, &end);

	double dur_ms = ((double)end.tv_sec - (double)start.tv_sec) * 1000 +
			((double)end.tv_nsec - (double)start.tv_nsec) / 1000000;
	printf("Plane allocation took %fms\n", dur_ms);
	printf("Plane allocation performed %zu atomic test commits\n",
	       liftoff_mock_commit_count);
	/* TODO: the mock libdrm library takes time to check atomic requests.
	 * This benchmark doesn't account for time spent in the mock library. */
	printf("With 20µs per atomic test commit, plane allocation would take "
	       "%fms\n", dur_ms + liftoff_mock_commit_count * 0.02);

	liftoff_device_destroy(device);
	close(drm_fd);
}
