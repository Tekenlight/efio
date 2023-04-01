#ifndef COMPRESSION_H_INCLUDED
#define COMPRESSION_H_INCLUDED

#include "zlib.h"

__BEGIN_DECLS

extern int compress_inp_buf(void *in, size_t in_size, void **out_ptr, size_t* written_size_ptr);

extern int uncompress_inp_buf(void *in, size_t in_size, void **out_ptr, size_t* written_size_ptr);

extern const char * zerr(int ret);

__END_DECLS
#endif // COMPRESSION_H_INCLUDED
