/*
 * REF: https://zlib.net/zlib_how.html
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "zlib.h"


#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

/* Compresses an input buffer to Z_DEFAULT_COMPRESSION level */
/* Compress from input memory buffer (in)
   compress_inp_buf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match. */
int compress_inp_buf(void *in, size_t in_size, void **out_ptr, size_t* written_size_ptr)
{
	int ret = 0;
	z_stream strm;
	size_t total_size = 0;

	/* allocate deflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
	if (ret != Z_OK)
		return ret;

	strm.avail_in = in_size;
	strm.next_in = in;
	/* run deflate() on input until output buffer not full, finish
	 * compression if all of source has been read in */
	int i = 0;
	*out_ptr = NULL;
	*written_size_ptr = 0;
	do {
		strm.avail_out = in_size;
		total_size = in_size * (++i);

		*out_ptr = realloc(*out_ptr, total_size);
		if (i == 1) strm.next_out = *out_ptr;

		ret = deflate(&strm, Z_FINISH);    /* no bad return value */
		assert(ret != Z_STREAM_ERROR);  /* state not clobbered */

		*written_size_ptr += in_size - strm.avail_out;

	} while (strm.avail_out == 0);
	assert(strm.avail_in == 0);     /* all input will be used */

	assert(ret == Z_STREAM_END);

	/* clean up and return */
	(void)deflateEnd(&strm);
	return Z_OK;
}

/* Decompress from input memory buffer
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match. */
int uncompress_inp_buf(void *in, size_t in_size, void **out_ptr, size_t* written_size_ptr)
{
	int ret = 0;
	size_t total_size = 0;
	z_stream strm;

	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit(&strm);
	if (ret != Z_OK)
		return ret;

	strm.avail_in = in_size;
	strm.next_in = in;

	/* run inflate() on input until output buffer not full */
	int i = 0;
	*out_ptr = NULL;
	void * temp_out = malloc(in_size);
	size_t written = 0;
	*written_size_ptr = 0;
	do {
		//memset(temp_out, 0, in_size);
		strm.avail_out = in_size;
		strm.next_out = temp_out;

		ret = inflate(&strm, Z_NO_FLUSH);

		assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
		switch (ret) {
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;     /* and fall through */
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				(void)inflateEnd(&strm);
			return ret;
		}

		written = in_size - strm.avail_out;

		total_size = in_size * (++i);
		*out_ptr = realloc(*out_ptr, total_size);

		//memset(((char*)(*out_ptr) + *written_size_ptr), 0, in_size);
		memcpy(((char*)(*out_ptr) + *written_size_ptr), temp_out, written);
		*written_size_ptr += written;

	} while (strm.avail_out == 0);
	free(temp_out);

	/* clean up and return */
	(void)inflateEnd(&strm);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

/* report a zlib or i/o error */
const char * zerr(int ret)
{
	char * error_ptr = NULL;
    fputs("zpipe: ", stderr);
    switch (ret) {
    case Z_ERRNO:
        error_ptr = "error in input/output";
        break;
    case Z_STREAM_ERROR:
        error_ptr = "invalid compression level";
        break;
    case Z_DATA_ERROR:
        error_ptr = "invalid or incomplete deflate data";
        break;
    case Z_MEM_ERROR:
        error_ptr = "out of memory";
        break;
    case Z_VERSION_ERROR:
        error_ptr = "zlib version mismatch!";
    }

	return error_ptr;
}


