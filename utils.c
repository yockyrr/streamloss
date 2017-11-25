#include <stdio.h>
#include <stdlib.h>

void sl_error(const char *user, const char *log, const char *file, int line) {
	fprintf(stderr, "Error: %s (%s at %s:%d)\n", user, log, file, line);
	return;
}

void sl_warning(const char *user, const char *log, const char *file,
	int line) {
	fprintf(stderr, "Warning: %s (%s at %s:%d)\n", user, log, file, line);
	return;
}
