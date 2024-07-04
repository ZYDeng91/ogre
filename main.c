#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <shout/shout.h>

#include "cfgparse.h"

int main(int argc, char **argv)
{
    shout_t *shout;
    unsigned char buff[4096];
    size_t read, total;
    int ret;

    if (argc != 2) 
    {
        fprintf(stderr, 
                "Usage: \"%s config.xml\"\n"
                "Using libshout %s\n", argv[0], shout_version(NULL, NULL, NULL));
        return 1;
    }

    shout_init();
    config_init();

    if (config_read(argv[1]) <= 0) 
    {
        fprintf(stderr, "Failed to read config file \"%s\"\n", argv[1]);
        goto fail;
    }

    if ((shout = shout_new()) == NULL) {
	fprintf(stderr, "Could not allocate shout_t\n");
	goto fail;
    }

    if (shout_set_host(shout, config_glob->instances->hostname) != SHOUTERR_SUCCESS) {
	fprintf(stderr, "Error setting host: %s\n", shout_get_error(shout));
	goto fail;
    }

    if (shout_set_port(shout, config_glob->instances->port) != SHOUTERR_SUCCESS) {
	fprintf(stderr, "Error setting port: %s\n", shout_get_error(shout));
	goto fail;
    }

    if (shout_set_password(shout, config_glob->instances->password) != SHOUTERR_SUCCESS) {
	fprintf(stderr, "Error setting password: %s\n", shout_get_error(shout));
	goto fail;
    }

    if (shout_set_mount(shout, config_glob->instances->mount) != SHOUTERR_SUCCESS) {
	fprintf(stderr, "Error setting mount: %s\n", shout_get_error(shout));
	goto fail;
    }

    if (config_glob->instances->user) shout_set_user(shout, config_glob->instances->user);

    // codecs (4th arg) is not implemented yet
    shout_set_content_format(shout, SHOUT_FORMAT_OGG, SHOUT_USAGE_AUDIO, NULL);

    shout_set_meta(shout, SHOUT_META_NAME, config_glob->stream_name);
    shout_set_meta(shout, SHOUT_META_GENRE, config_glob->stream_genre);
    shout_set_meta(shout, SHOUT_META_DESCRIPTION, config_glob->stream_description);
    if (config_glob->stream_url) shout_set_meta(shout, SHOUT_META_URL, config_glob->stream_url);

    if (shout_open(shout) == SHOUTERR_SUCCESS) {
	printf("Connected to server\n");
	total = 0;
	while (1) {
	    read = fread(buff, 1, sizeof(buff), stdin);
	    total += read;

	    if (read > 0) {
		ret = shout_send(shout, buff, read);
		if (ret != SHOUTERR_SUCCESS) {
		    fprintf(stderr, "Send error: %s\n", shout_get_error(shout));
		    break;
		}
	    } else {
		break;
	    }

	    shout_sync(shout);
	}
	printf("Total bytes read: %lu\n", total);
    } else {
	fprintf(stderr, "Error connecting: %s\n", shout_get_error(shout));
	goto fail;
    }

    shout_close(shout);

  // use goto or free resources every time, or neither
  // this might be one rare occasion where goto is good (i think)
  fail:
    shout_shutdown();
    config_shutdown();

    return 0;
}
