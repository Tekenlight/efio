#include <assert.h>
#include <ev_buffered_stream.h>

ev_buffered_stream::ev_buffered_stream(chunked_memory_stream * memory_stream, std::streamsize bufsize, std::streamsize max_len):
		_bufsize(bufsize),
		_p_buffer(0),
		_w_buffer(0),
		_mode(ios::out|ios::in),
		_memory_stream(memory_stream),
		_nodeptr(0),
		_prev_len(0),
		_max_len(max_len),
		_cum_len(0)
{
	this->setg(_p_buffer, _p_buffer, _p_buffer);
	if(!_w_buffer) {
		_w_buffer = (char*)malloc(BUFFER_SIZE);
		memset(_w_buffer,0,BUFFER_SIZE);
	}
	this->setp(_w_buffer, _w_buffer+BUFFER_SIZE);
	if (0 == _max_len) _max_len = -1;
}

ev_buffered_stream::~ev_buffered_stream()
{
	flush_buffer();
	free(_w_buffer);
	_w_buffer = NULL;
}

int ev_buffered_stream::overflow(int c)
{
	if (!(_mode & ios::out)) return EOF;
	//printf("%s:%d reached here\n",__FILE__, __LINE__);

	if (flush_buffer() == std::streamsize(-1)) return EOF;
	if (c != EOF) {
		*this->pptr() = (char)(c);
		this->pbump(1);
	}

	return c;
}

int ev_buffered_stream::underflow()
{
	//puts("underflow called");
	if (!(_mode & ios::in)) return EOF;

	//printf("%s:%d reached here\n",__FILE__, __LINE__);
	if (this->gptr() && (this->gptr() < this->egptr()))
		return int(*this->gptr());

	//printf("%s:%d reached here\n",__FILE__, __LINE__);
	// n will be <= _bufsize
	size_t n = read_from_source(_bufsize);
	assert(n <=_bufsize);
	if (n <= 0) return EOF;

	this->setg(_p_buffer, _p_buffer, _p_buffer + n);

	// return next character
	return (int)(*this->gptr());    
}

int ev_buffered_stream::sync()
{
	if (this->pptr() && this->pptr() > this->pbase()) {
		//printf("%s:%d reached here\n",__FILE__, __LINE__);
		//puts(_w_buffer);
		if (flush_buffer() == -1) return -1;
	}

	return 0;
}


void ev_buffered_stream::setMode(openmode mode)
{
	_mode = mode;
}

ev_buffered_stream::openmode ev_buffered_stream::getMode() const
{
	return _mode;
}

size_t ev_buffered_stream::read_from_source(std::streamsize size)
{
	size_t len = 0;

	//printf("%s:%d reached here %zu %zd\n",__FILE__, __LINE__, _prev_len, _max_len);
	_memory_stream->erase(_prev_len);
	if ((-1 == _max_len) || (_cum_len < _max_len)) {
		//puts("read_from_source got called");

		//printf("_p_buffer = [%p]\n",_p_buffer);

		if ((-1 != _max_len) && ((_cum_len + size) > _max_len))
			size = _max_len - _cum_len;

		_nodeptr = _memory_stream->get_next(0);

		if(_nodeptr) {
			len = _memory_stream->get_buffer_len(_nodeptr);
			_p_buffer = (char*)_memory_stream->get_buffer(_nodeptr);
			/* This is in order to limit the size read from the source
			 * So that if there is any logic by the caller that after certain
			 * length the stream should return EOF, the stream buffer
			 * wil behave accordingly.
			 * */
			if (len < 0) len = 0;
			if (len > size) len = size;
		}
		else {
			_p_buffer = NULL;
			len = 0;
		}

		//printf("Length = [%lu]\n",len);
		_cum_len += len;
	}
	_prev_len = len;
	return len;
}

size_t ev_buffered_stream::push_to_sync(char *buffer, std::streamsize size)
{
	//printf("%s:%d reached here\n",__FILE__, __LINE__);
	size_t len = size;
	if ((-1 == _max_len) || (_cum_len < _max_len)) {
		if ((-1 != _max_len) && ((_cum_len + len) > _max_len))
			len = _max_len - _cum_len;

		_memory_stream->push(buffer, len);

		_cum_len += len;
	}
	return len;
}

void ev_buffered_stream::post_write_buffer(char* buffer, std::streamsize bytes, char **buffer_ptr, size_t *bytes_ptr)
{
	*buffer_ptr = 0;
	*bytes_ptr = 0;

	return;
}

void ev_buffered_stream::pre_write_buffer(char* buffer, std::streamsize bytes, char **buffer_ptr, size_t *bytes_ptr)
{
	*buffer_ptr = 0;
	*bytes_ptr = 0;

	return;
}

size_t ev_buffered_stream::write_to_sync(char *buffer, std::streamsize bytes)
{
	//printf("%s:%d reached here %zu\n",__FILE__, __LINE__, bytes);
	//puts(buffer);
	size_t transfered = 0;
	{
		char * pre_buffer = 0;
		size_t pre_bytes = 0;
		pre_write_buffer(buffer, bytes, &pre_buffer, &pre_bytes);
		if ((pre_bytes) && (pre_buffer)) {
			transfered += push_to_sync(pre_buffer, pre_bytes);
		}
	}

	transfered += push_to_sync(buffer, bytes);

	{
		char * post_buffer = 0;
		size_t post_bytes = 0;
		post_write_buffer(buffer, bytes, &post_buffer, &post_bytes);
		if ((post_bytes) && (post_buffer)) {
			transfered += push_to_sync(post_buffer, post_bytes);
		}
	}

	return transfered;
}

int ev_buffered_stream::flush_buffer()
{
	if (!(_mode & ios::out)) return EOF;
	if (!_w_buffer) return -1;

	int n = int(this->pptr() - this->pbase());

	if (n) {
		//printf("%s:%d reached inside flush_buffer here\n",__FILE__, __LINE__);
		//printf ("_mode = %x, ios::out = %x, ios::in = %x _w_buffer = %p\n",_mode, ios::out, ios::in, _w_buffer);

		if (write_to_sync(this->pbase(), n) == n) {
			//printf("%s:%d reached inside flush_buffer here\n",__FILE__, __LINE__);
			_w_buffer = (char*)malloc(BUFFER_SIZE);
			memset(_w_buffer,0,BUFFER_SIZE);
			this->setp(_w_buffer, _w_buffer+BUFFER_SIZE);
			return n;
		}
	}

	return -1;
}
