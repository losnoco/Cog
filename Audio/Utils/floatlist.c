#include <stdint.h>
#include <stdio.h>

int main(void) {
	fprintf(stdout, "static const float dsdtofloat[256][8] = {\n");

	for(size_t i = 0; i < 256; ++i) {
		fprintf(stdout, "\t{ ");
		for(size_t j = 0; j < 8; ++j) {
			if(j) fprintf(stdout, ", ");
			fprintf(stdout, "%s", ((i << j) & 128) ? "+1.0f" : "-1.0f");
		}
		fprintf(stdout, " }%s", (i < 255) ? ",\n" : "\n");
	}

	fprintf(stdout, "};\n");

	return 0;
}
