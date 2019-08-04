#include <chunked_memory_stream.h>
#include <string.h>

chunked_memory_stream::chunked_memory_stream()
{
}

// Transfers 'bytes' number of bytes to the chunked_memory_stream.
// From the memory buffer.
// The caller is expected to manage the memory for buffer.
// Returns the number of bytes transferred.
int chunked_memory_stream::push(void * buffer, size_t bytes)
{
	_buffer_list.add_node(buffer,bytes);
	return 0;
}

// Copies 'bytes' number of bytes from the chunked_memory_stream,
// the data is copied starting at offset '0 + start_pos'.
// If there is less data, as many bytes as there are are copied
// to the location pointed by buffer.
// The caller is expected to allocate memory to the buffer.
//
// The index start_pos is C style, i.e. first character is at 0th
// position.
//
// Differences between this and pull_out are,
// . This method only copies the data and leaves the source unaltered
// . This method has the ability to copy from any offset location.
//
// Returns the number of bytes copied, or 0 if no data is available
// or -1 if there is any error.
size_t chunked_memory_stream::read(size_t start_pos, void *buffer, size_t bytes)
{
	memory_buffer_list::node * node_ptr = 0;
	void * node_buffer = 0;
	size_t to_be_copied = 0;
	size_t copied = 0;
	size_t traversed = 0; // Traversed indicates count and not index.
	size_t start_pos_in_node = 0;

	node_ptr = _buffer_list.get_head();
	while (node_ptr) {
		// First reach the node to start copying from
		if ((start_pos - traversed) >= node_ptr->get_buffer_len()) {
			traversed += node_ptr->get_buffer_len();
			node_ptr = _buffer_list.get_next(node_ptr);
			continue;
		}
		// offset within the node
		//
		// 01->23->45->67
		// traversed can be 0, or 2, or 4, or 6
		// if start_pos is, say 4, traversed will be 4 (2 +2)
		// start_pos_in_node is this case should be 0 (= 4 - 4)
		//
		// If start_pos is 5 start_pos_in_node should be 1 and will be
		// (= 5 - 4).
		// start_pos_in_node is also an index variable
		// Thus starts from 0
		start_pos_in_node = start_pos - traversed;
		traversed += start_pos_in_node;
		break;
	}

	// Start position is beyond the total buffer.
	if (!node_ptr) return -1;

	// copy as much data required.
	while (node_ptr && copied < bytes) {
		to_be_copied = 0; // To be copied from the current node.

		// => start_pos is within this buffer.
		if ((bytes - copied) >= (node_ptr->get_buffer_len() - start_pos_in_node)) {
			// remaining to be copied exceeds the buffer len.
			// Copy what can be taken from the buffer.
			to_be_copied = node_ptr->get_buffer_len() - start_pos_in_node;
		}
		else {
			// remaining to be copied is not exceeding the buffer len.
			to_be_copied = bytes - copied;
		}
		node_buffer = node_ptr->get_buffer();
		memcpy(((char*)buffer + copied), ((char*)node_buffer + start_pos_in_node)  , to_be_copied);

		copied += to_be_copied;
		// Next buffer onwards offset to copy from will be from
		// begining.
		start_pos_in_node = 0;
		node_ptr = _buffer_list.get_next(node_ptr);
	}
	return copied;
}

// Copies 'bytes' number of bytes from the chunked_memory_stream,
// from the start position 0.
// If there is less data, as many bytes as there are are copied
// to the location pointed by buffer.
// The caller is expected to allocate memory to the buffer.
//
// Unlike the other version of read, this copies the data to the buffer
// and erases from the source.
//
// The index start_pos is C style, i.e. first character is at 0th
// position.
//
// Differences between this and pull_out are,
// . This method only copies the data and leaves the source unaltered
// . This method has the ability to copy from any offset location.
//
// Returns the number of bytes copied, or 0 if no data is available
// or -1 if there is any error.
size_t chunked_memory_stream::read(void *buffer, size_t bytes)
{
	memory_buffer_list::node * node_ptr = 0;
	void * node_buffer = 0;
	size_t to_be_copied = 0;
	size_t copied = 0;
	size_t traversed = 0; // Traversed indicates count and not index.

	node_ptr = _buffer_list.get_head();
	if (!node_ptr) return 0;

	// copy as much data required.
	while (node_ptr && copied < bytes) {
		to_be_copied = 0; // To be copied from the current node.

		// => start_pos is within this buffer.
		if ((bytes - copied) >= node_ptr->get_buffer_len()) {
			// remaining to be copied exceeds the buffer len.
			// Copy what can be taken from the buffer.
			to_be_copied = node_ptr->get_buffer_len();
		}
		else {
			// remaining to be copied is not exceeding the buffer len.
			to_be_copied = bytes - copied;
		}
		node_buffer = node_ptr->get_buffer();
		memcpy(((char*)buffer + copied), (char*)node_buffer  , to_be_copied);

		if (to_be_copied == node_ptr->get_buffer_len())
			_buffer_list.pop_head();
		else {
			node_ptr->shift_buffer_position(to_be_copied);
		}
		copied += to_be_copied;
		node_ptr = _buffer_list.get_head();
	}

	return copied;
}

// Moves the head of the data stream to the offset 0 + bytes
// Memory holding the data is freed.
// Returns the number of bytes erased.
size_t chunked_memory_stream::erase(size_t bytes)
{
	memory_buffer_list::node * node_ptr = 0;
	void * node_buffer = 0;
	size_t to_be_erased = 0;
	size_t erased = 0;
	size_t buffer_len = 0;

	to_be_erased = bytes;
	node_ptr = _buffer_list.get_head();
	while (to_be_erased && node_ptr) {
		// First reach the node to start copying from
		if (to_be_erased >= node_ptr->get_buffer_len()) {
			to_be_erased -= node_ptr->get_buffer_len();
			erased += node_ptr->get_buffer_len();
			_buffer_list.pop_head();
			node_ptr = _buffer_list.get_head();
			continue;
		}

		buffer_len = node_ptr->get_buffer_len();
		node_buffer = node_ptr->get_buffer();
		for (int i = 0; i < (buffer_len - to_be_erased) ; i++) {
			*((char*)(node_buffer) + i) = *((char*)(node_buffer) + to_be_erased + i);
		}
		buffer_len -= to_be_erased;
		erased += to_be_erased;
		node_ptr->set_buffer(node_buffer, buffer_len);
		to_be_erased = 0;

		break;
	}

	return erased;
}

// Gets the available allocated, buffer at the head.
// This way of accessing data will help to avoid
// a memcpy
void * chunked_memory_stream::get_buffer()
{
	memory_buffer_list::node * head_ptr = _buffer_list.get_head();
	return (head_ptr)?_buffer_list.get_head()->get_buffer():0;
}

// Gets length of the available allocated buffer at the head.
// This way of accessing data will help to avoid
size_t chunked_memory_stream::get_buffer_len()
{
	memory_buffer_list::node * head_ptr = _buffer_list.get_head();
	return (head_ptr)?_buffer_list.get_head()->get_buffer_len():0;
}

size_t chunked_memory_stream::get_buffer_len(void * nodeptr)
{
	if (nodeptr)
		return ((memory_buffer_list::node *)(nodeptr))->get_buffer_len();
	else
		return 0;
}

void * chunked_memory_stream::get_buffer(void * nodeptr)
{
	if (nodeptr)
		return ((memory_buffer_list::node *)(nodeptr))->get_buffer();
	else
		return 0;
}

void * chunked_memory_stream::get_next(void * nodeptr)
{
	return (!nodeptr) ? (void*)(_buffer_list.get_head()) : (void*)(_buffer_list.get_next(((memory_buffer_list::node *)nodeptr))) ;
}

chunked_memory_stream::~chunked_memory_stream()
{
	//printf("In chunked_memory_stream destructor\n");
}

