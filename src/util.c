#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "util.h"

void handle_error(int err) {
	if (err < 0) {
		fprintf(stderr, "%s\n", strerror(errno));
		exit(0);
	}
}
