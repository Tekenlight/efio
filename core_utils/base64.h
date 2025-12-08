#ifndef BASE64_H_INCLUDED
#define BASE64_H_INCLUDED

unsigned char *base64_encode(const unsigned char *data, size_t input_length,
                                size_t *output_length, int add_line_breaks);
unsigned char *base64_decode(const unsigned char *data, size_t input_length,
                                                        size_t *output_length);

unsigned char *url_base64_encode(const unsigned char *data, size_t input_length,
								size_t *output_length, int add_line_breaks);
unsigned char *url_base64_decode(const unsigned char *data, size_t input_length,
                                                        size_t *output_length);
#endif
