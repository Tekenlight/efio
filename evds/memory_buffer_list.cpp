#include <stdlib.h>
#include <assert.h>
#include <memory_buffer_list.h>


memory_buffer_list::node::node(): _next(0),_buffer(0),_size(0),_buffer_position_ptr(0),_size_at_position(0)
{
}

memory_buffer_list::node * memory_buffer_list::node::get_next()
{
	node *next = 0;
	next = (node*)std::atomic_load(&_next);
	return next;
}

void memory_buffer_list::node::set_buffer(void * buffer, size_t size)
{
	_buffer = buffer;
	_size = size;
	_buffer_position_ptr = _buffer;
	_size_at_position = _size;
}

void memory_buffer_list::node::set_next(node * np)
{
	std::atomic_store(&_next,(uintptr_t)np);
}

size_t memory_buffer_list::node::get_buffer_len()
{
	return _size_at_position;
}

void * memory_buffer_list::node::get_buffer()
{
	return _buffer_position_ptr;
}

void memory_buffer_list::node::shift_buffer_position(size_t bytes)
{
	//printf("BPP %ld bytes %zu buffer %ld size %zu\n",_buffer_position_ptr, bytes, _buffer, _size);
	assert(((char*)_buffer_position_ptr + bytes) <= ((char*)_buffer + _size));

	if (((char*)_buffer_position_ptr + bytes) < ((char*)_buffer + _size)) {
		_buffer_position_ptr = (void*)((char*)_buffer_position_ptr + bytes);
		_size_at_position -= bytes;
	}
	else {
		_buffer_position_ptr = NULL;
		_size_at_position = 0;
	}
	//printf("BPP %ld bytes %zu buffer %ld size %zu\n",_buffer_position_ptr, bytes, _buffer, _size);
}


memory_buffer_list::node::~node()
{
	//printf("In node destructor\n");
	free(_buffer);
	_buffer = 0;
	_size = 0;
	_next = 0;
	_buffer_position_ptr = NULL;
	_size_at_position = 0;
}

memory_buffer_list::memory_buffer_list():_head(0),_tail(0)
{
}

memory_buffer_list::node * memory_buffer_list::get_head()
{
	node * head;
	head = (node*)std::atomic_load(&_head);
	return head;
}

memory_buffer_list::node * memory_buffer_list::get_next(node * ptr)
{
	return ptr->get_next();
}

memory_buffer_list::node * memory_buffer_list::get_tail()
{
	node * tail;
	tail = (node*)std::atomic_load(&_tail);
	return tail;
}

void memory_buffer_list::add_node(void * buffer, size_t size)
{
	//printf("%s:%d Here bytes = %zu buffer = %p\n%s", __FILE__,__LINE__,size,buffer,(char*)buffer);
	node * np = new node();
	uintptr_t unp = (uintptr_t)np;
	node * old_tail = 0;
	//printf("%s:%d Here bytes = %zu buffer = %p\n", __FILE__,__LINE__,size,buffer);
	np->set_buffer(buffer,size);
	//printf("%s:%d Here bytes = %zu buffer = %p\n", __FILE__,__LINE__,size,buffer);
	np->set_next(0);
	//printf("%s:%d Here bytes = %zu buffer = %p\n", __FILE__,__LINE__,size,buffer);
	old_tail = (node*)std::atomic_exchange(&_tail,(uintptr_t)unp);
	/* For a brief period of time,
	 * 1. When the new tail node is being added there can be a discontinuity in the list.
	 * 2. Head can become null temporarily when the first record is being added.
	 * */
	//printf("%s:%d Here bytes = %zu buffer = %p\n", __FILE__,__LINE__,size,buffer);
	if (old_tail) {
	//printf("%s:%d Here bytes = %zu buffer = %p\n", __FILE__,__LINE__,size,buffer);
		old_tail->set_next(np);
	}
	else {
		//std::atomic_exchange(&_head , _tail);
		_head.exchange(_tail);
	}

	return;
}

memory_buffer_list::node * memory_buffer_list::pop_head()
{
	node * hp = 0;
	node * tail = (node*)std::atomic_load(&_tail);
	node * next = 0;
	/* This is a variation from the generic queue implementation,
	 * because
	 * 1. Generic queue assumes concurrent threads doing push and pop arbitrarily.
	 * 2. This queue provides for ony 2 threads concurrently acting
	 *    one only either reading or popping and another only pushing.
	 * Thus once we know that tail is not null, we can assume that
	 * no other thread will make it null.
	 * */
	if (tail) {
		/* Since there is tail, the list is not empty
		 * head can become null temporarily. we have to
		 * wait for head to become available. And again
		 * once it is available, no other thread will pop it out
		 * therefore no need for marking the head etc.
		 * */
		hp = (node*)std::atomic_load(&_head);
		while (!hp) { EV_YIELD(); hp = (node*)std::atomic_load(&_head); }
		//EV_DBGP(" hp = [%p]\n",hp);
		next = get_next(hp);
		if (!next) {
			bool flg = false;
			uintptr_t newh = (uintptr_t)hp;
			//flg = std::atomic_compare_exchange_strong(&_tail,&newh,0);
			flg = _tail.compare_exchange_strong(newh,0);
			if (!flg) {
				// The case when one thread popped out the single node, also the head
				// and the other thread added concurrently
				do {
					EV_YIELD();
					next = get_next(hp);
				} while (!next);
			}
		}
		{
			uintptr_t h = (uintptr_t)hp;
			/* It is quite possible that by the time we want to set the head
			 * the value of _head could have changed, due to the fact that
			 * we set tail to 0 above in the compare exchange operation.
			 * The add_node method thought that the queue is empty and
			 * set a new value of _head.
			 * However if it happens to have not changed from what was found
			 * initially here, we should change it to the next pointer in
			 * the chain.
			 * */
			std::atomic_compare_exchange_strong(&_head,&h,(uintptr_t)next);
		}
	}
	return hp;
}

memory_buffer_list::~memory_buffer_list()
{
	//printf("in memory_buffer_list destructor\n");
	node * p = 0, *q = 0;
	p = (node*)std::atomic_load(&_head);
	while (p != 0) {
		q = p->get_next();
		delete p;
		p = q;
	}
}

