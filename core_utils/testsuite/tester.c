#include <stdio.h>
#include <stdlib.h>
#include "ef_compression.h"

#define CHUNK 16384

int main()
{
	FILE * fp = fopen("Makefile", "r");
	unsigned char in[CHUNK];

	size_t sz = fread(in, 1, CHUNK, fp);

	FILE * out_fp = fopen("Makefile.z", "w");

	void * compressed_data = NULL;
	size_t output_size = 0;

	compress_inp_buf(in, sz, &compressed_data, &output_size);

	fwrite(compressed_data, 1, output_size, out_fp);

	printf("Output size = [%zd]\n", output_size);

	fclose(fp);
	fclose(out_fp);

	void * uncompressed_data  = NULL;

	uncompress_inp_buf(compressed_data, output_size, &uncompressed_data, &output_size);
	fp = fopen("new.Makefile", "w");
	fwrite(uncompressed_data, 1, output_size, fp);
	fclose(fp);



	free(compressed_data);
	free(uncompressed_data);

	return 0;
}
