#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void free_binary_data(unsigned char *data);
size_t binary_data_len(unsigned char *data);
void * alloc_binary_data_memory(size_t size);

static unsigned char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
								'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
								'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
								'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
								'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
								'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
								'w', 'x', 'y', 'z', '0', '1', '2', '3',
								'4', '5', '6', '7', '8', '9', '+', '/'
};


static unsigned char decoding_table[256] = {
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3e, 0xFF, 0xFF, 0xFF, 0x3f,
            0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
            0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
            0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static int mod_table[] = {0, 2, 1};

/*
 * ### unsigned char *base64_encode(const unsigned char *data, size_t input_length,
                                size_t *output_length, int add_line_breaks)
 *
 * DESCRIPTION:
 *              Encodes the given input binary data to base64 string format.
 *              Memory needed for the returned string is allocated within and the calling functions is
 *              expected to free the memory.
 * INPUT:
 *              const unsigned char *data : binary data
 *              size_t input_length       : size of the binary data
 *              size_t *output_length     : pointer to variable of type size_t where the length of the output
 *                                          buffer will be written
 * OUTPUT:
 *              output_length
 *              unsigned char *           : output buffer
 */
unsigned char *base64_encode(const unsigned char *data, size_t input_length,
								size_t *output_length, int add_line_breaks)
{

	int line_count = 0;

	*output_length = 4 * ((input_length + 2) / 3);
	if (add_line_breaks) { //  CRLF after each 76 chars
		*output_length += (*output_length / 76) * 2;
	}

	unsigned char *encoded_data = malloc(*output_length + 1);
	if (encoded_data == NULL) return NULL;
	memset(encoded_data, 0, (*output_length + 1));

	for (int i = 0, j = 0; i < input_length;) {

		uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
		uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
		uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

		uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

		encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
		if (add_line_breaks) {
			if (++line_count == 19) {
				encoded_data[j++] = 13;
				encoded_data[j++] = 10;
				line_count = 0;
			}
		}
	}

	for (int i = 0; i < mod_table[input_length % 3]; i++) {
		encoded_data[*output_length - 1 - i] = '=';
	}

	return encoded_data;
}

#define ISSPACE(c) (c==10 || c==13 || c==32 || c==9)
#define BASE64_PAD '='

/*
 * ### unsigned char *base64_decode(const unsigned char *data, size_t input_length, size_t *output_length)
 *
 * DESCRIPTION:
 *              Decodes the input base64 encoded string
 *              Memory needed for the returned binary data is allocated within and the calling functions is
 *              expected to free the memory.
 * INPUT:
 *              const unsigned char *data : string data
 *              size_t input_length       : size of the string data
 *              size_t *output_length     : pointer to variable of type size_t where the length of the output
 *                                          buffer will be written
 * OUTPUT:
 *              output_length
 *              unsigned char *           : output buffer
 */
unsigned char *base64_decode(const unsigned char *data, size_t input_length, size_t *output_length)
{
	int i = 0, out_index = 0, state = 0, ch = 0;
	uint32_t sextet = 0;
	unsigned char *decoded_data = NULL;
	size_t estimated_output_length = 0;

	estimated_output_length = ((input_length / 4) * 3);
	decoded_data = alloc_binary_data_memory(estimated_output_length);
	if (decoded_data == NULL) return NULL;

	*output_length = 0;


	state = 0;
	out_index = 0;

	for (;i<input_length; i++) {

		ch = data[i];

		if (ISSPACE(ch))  {/* Skip whitespace anywhere. */
			continue;
		}

		if (ch == BASE64_PAD) {/* BASE64_PAD will appear only in the end */
			break;
		}

		sextet = 0;
		sextet = decoding_table[ch];
		if (sextet == 0xFF) {
			free_binary_data(decoded_data);
			return NULL;
		}

		switch (state) {
			case 0:
				if (decoded_data) {
					if ((size_t)out_index >= estimated_output_length) {
						free_binary_data(decoded_data);
						return NULL;
					}
					decoded_data[out_index] = sextet << 2;
				}
				state = 1;
				break;
			case 1:
				if (decoded_data) {
					if ((size_t)out_index + 1 >= estimated_output_length) {
						free_binary_data(decoded_data);
						return NULL;
					}
					decoded_data[out_index] |= sextet >> 4;
					decoded_data[out_index + 1] = (sextet & 0x0f) << 4;
				}
				out_index++;
				state = 2;
				break;
			case 2:
				if (decoded_data) {
					if ((size_t)out_index + 1 >= estimated_output_length) {
						free_binary_data(decoded_data);
						return NULL;
					}
					decoded_data[out_index] |= sextet >> 2;
					decoded_data[out_index + 1] = (sextet & 0x03) << 6;
				}
				out_index++;
				state = 3;
				break;
			case 3:
				if (decoded_data) {
					if ((size_t)out_index >= estimated_output_length) {
						free_binary_data(decoded_data);
						return NULL;
					}
					decoded_data[out_index] |= sextet;
				}
				out_index++;
				state = 0;
				break;
			default:
				free_binary_data(decoded_data);
				return NULL;
		}
	}
	(*output_length) = out_index;

	/*
	* We are done decoding Base-64 chars.  Let's see if we ended
	* on a byte boundary, and/or with erroneous trailing characters.
	*/

	if (ch == BASE64_PAD) {
		/* We got a pad char. */
		ch = data[i++]; /* Skip it, get next. */
		switch (state) {
			case 0: /* Invalid = in first position */
			case 1: /* Invalid = in second position */
				free_binary_data(decoded_data);
				return NULL;

			case 2: /* Valid, means one byte of info */
					/* Skip any number of spaces. */
				for ((void)NULL; ((ch != '\0')&&(i<input_length)); ch = data[i++]) {
					if (!ISSPACE(ch))
						break;
				}
				/* Make sure there is another trailing = sign. */
				if (ch != BASE64_PAD) {
					free_binary_data(decoded_data);
					return NULL;
				}
				ch = data[i++]; /* Skip the = */
							 /* Fall through to "single trailing =" case. */
							 /* FALLTHROUGH */

			case 3: /* Valid, means two bytes of info */
					/*
						* We know this char is an =.  Is there anything but
						* whitespace after it?
						*/
				for ((void)NULL; ((ch != '\0')&&(i<input_length)); ch = data[i++]) {
					if (!ISSPACE(ch) && (ch != 0)) {
						free_binary_data(decoded_data);
						return NULL;
					}
				}

				/*
				* Now make sure for cases 2 and 3 that the "extra"
				* bits that slopped past the last full byte were
				* zeros.  If we don't check them, they become a
				* subliminal channel.
				*/
				if (decoded_data && decoded_data[out_index] != 0) { 
					free_binary_data(decoded_data);
					return NULL;
				}
		}
	}
	else {
		/*
		* We ended by seeing the end of the string.  Make sure we
		* have no partial bytes lying around.
		*/
		if (state != 0) {
			free_binary_data(decoded_data);
			return NULL;
		}
	}

	return (decoded_data);
}

#if 0
int main()
{

    const unsigned char * data = (const unsigned char *)"Hello World Sriram and Gowri !!!"
    													"Hello World Sriram and Gowri !!!!"
    													"Hello World Sriram and Gowri !!!";
    size_t input_size = strlen((const char*)data);
    printf("Input size: %ld \n",input_size);
    unsigned char * encoded_data = base64_encode(data, input_size, &input_size, 1);
    printf("After size: %ld \n",input_size);
    printf("Encoded Data is: %s \n",encoded_data);

    size_t decode_size = strlen((const char*)encoded_data);
    printf("Output size: %ld \n",decode_size);
    unsigned char * decoded_data = base64_decode(encoded_data, decode_size, &decode_size);
    printf("After size: %ld \n",decode_size);
    printf("Decoded Data is: %s \n",decoded_data);
    return 0;
}

#elif 0
int main()
{

    const unsigned char data [] = {0xFF, 0xA1, 0x12, 0xBE }; 
    size_t input_size = strlen((const char*)data);
    printf("Input size: %ld \n",input_size);
    unsigned char * encoded_data = base64_encode(data, input_size, &input_size, 1);
    printf("After size: %ld \n",input_size);
    printf("Encoded Data is:%s\n",encoded_data);

    size_t decode_size = strlen((const char*)encoded_data);
    printf("Output size: %ld \n",decode_size);
    unsigned char * decoded_data = base64_decode(encoded_data, decode_size, &decode_size);
    printf("After size: %ld \n",decode_size);
    printf("Decoded Data is:");
	int i =0;
	while (i < decode_size) {
		printf("%X", decoded_data[i]);
		i++;
	}
	printf("\n");
    return 0;
}
#endif

