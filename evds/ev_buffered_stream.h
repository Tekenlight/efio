#include <chunked_memory_stream.h>
#include <streambuf>
#include <iosfwd>
#include <ios>

#ifndef EV_BUFFERED_stream
#define EV_BUFFERED_stream
template <typename ch>
class buffer_allocator
{
public:
	typedef ch char_type;

	static char_type* allocate(std::streamsize size)
	{
		return new char_type[static_cast<std::size_t>(size)];
	}

	
	static void deallocate(char_type* ptr, std::streamsize /*size*/) throw()
	{
		delete [] ptr;
	}

};


class ev_buffered_stream: public std::basic_streambuf<char, std::char_traits<char> >
{
protected:
	typedef std::basic_ios<char, std::char_traits<char> > ios;
	typedef ios::openmode openmode;

public:
	//ev_buffered_stream(std::streamsize bufferSize, openmode mode = ios::in);
	ev_buffered_stream(chunked_memory_stream * memory_stream, std::streamsize bufsize, std::streamsize max_to_read = -1);

	~ev_buffered_stream();

	virtual int overflow(int c);

	virtual int underflow();

	virtual int sync();

	void setMode(openmode mode);

	openmode getMode() const;

private:
	virtual size_t read_from_source(std::streamsize);

	virtual size_t write_to_sync(char * , std::streamsize);

	int flush_buffer();

	std::streamsize			_bufsize;
	char*					_pBuffer;
	openmode				_mode;
	chunked_memory_stream*	_memory_stream;
	void*					_nodeptr;
	size_t					_prev_len;
	size_t					_max_to_read;
	size_t					_cum_len;

	ev_buffered_stream(const ev_buffered_stream&);
	ev_buffered_stream& operator = (const ev_buffered_stream&);
};

#endif
