#include <assert.h>
#include <ev_buffered_stream.h>

ev_buffered_stream::ev_buffered_stream(chunked_memory_stream * memory_stream, std::streamsize bufsize, std::streamsize max_to_read):
		_bufsize(bufsize),
		_pBuffer(0),
		_mode(ios::in),
		_memory_stream(memory_stream),
		_nodeptr(0),
		_prev_len(0),
		_max_to_read(max_to_read),
		_cum_len(0)
{
	this->setg(_pBuffer, _pBuffer, _pBuffer);
}

ev_buffered_stream::~ev_buffered_stream() { }

int ev_buffered_stream::overflow(int c)
{
	return EOF;
}

/*
int ev_buffered_streamev_buffered_stream::overflow(int c)
{
	if (!(_mode & ios::out)) return EOF;

	if (flush_buffer() == std::streamsize(-1)) return EOF;
	if (c != EOF) {
		*this->pptr() = (char)(c);
		this->pbump(1);
	}

	return c;
}
*/

int ev_buffered_stream::underflow()
{
	//puts("underflow called");
	if (!(_mode & ios::in)) return EOF;

	if (this->gptr() && (this->gptr() < this->egptr()))
		return int(*this->gptr());

	// n will be <= _bufsize
	size_t n = read_from_source(_bufsize);
	assert(n <=_bufsize);
	if (n <= 0) return EOF;

	this->setg(_pBuffer, _pBuffer, _pBuffer + n);

	// return next character
	return (int)(*this->gptr());    
}

int ev_buffered_stream::sync()
{
	if (this->pptr() && this->pptr() > this->pbase()) {
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

	_memory_stream->erase(_prev_len);
	if ((-1 == _max_to_read) || (_cum_len < _max_to_read)) {
		//puts("read_from_source got called");

		//printf("_pBuffer = [%p]\n",_pBuffer);

		if ((-1 != _max_to_read) && ((_cum_len + size) > _max_to_read))
			size = _max_to_read - _cum_len;

		_nodeptr = _memory_stream->get_next(0);

		if(_nodeptr) {
			len = _memory_stream->get_buffer_len(_nodeptr);
			_pBuffer = (char*)_memory_stream->get_buffer(_nodeptr);
			/* This is in order to limit the size read from the source
			 * So that if there is any logic by the caller that after certain
			 * length the stream should return EOF, the stream buffer
			 * wil behave accordingly.
			 * */
			if (len < 0) len = 0;
			if (len > size) len = size;
		}
		else {
			_pBuffer = NULL;
			len = 0;
		}

		//printf("Length = [%lu]\n",len);
		_cum_len += len;
	}
	_prev_len = len;
	return len;
}

size_t ev_buffered_stream::write_to_sync(char * , std::streamsize)
{
	return 0;
}

int ev_buffered_stream::flush_buffer()
{
	int n = int(this->pptr() - this->pbase());
	if (write_to_sync(this->pbase(), n) == n) {
		this->pbump(-n);
		return n;
	}
	return -1;
}
