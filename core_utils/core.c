#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void free_binary_data(unsigned char *data)
{
	//unsigned char *address = data - sizeof(size_t);
	//printf("%s:%d, Freeing %p:%p, size[%lu]\n", __FILE__, __LINE__, (void*) address, (void*)data, sizeof(size_t));
	//free(address);
	free(data);
}

size_t binary_data_len(unsigned char *data)
{
	//size_t size;
	//memcpy(&size, (unsigned char *)data - sizeof(size_t), sizeof(size_t));
	//return size;
	return -1;
}

void * alloc_binary_data_memory(size_t size)
{
	//void* data = malloc(size + sizeof(size_t) + 1);
	void* data = malloc(size + 1);

	if (data == NULL) return NULL;
	memset(data, 0, size+1);

	//memcpy(data, &size, sizeof(size_t));
	//data = data + sizeof(size_t);

	return data;
}

