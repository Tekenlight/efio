#include <chunked_memory_stream.h>
#include <streambuf>
#include <iosfwd>
#include <ios>

#include <string.h>

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

	static const int BUFFER_SIZE = 4096;
	size_t low_read_from_source(std::streamsize);

	size_t low_write_to_sync(char * , std::streamsize);

	int flush_buffer();


public:
	//ev_buffered_stream(std::streamsize bufferSize, openmode mode = ios::in);
	ev_buffered_stream(chunked_memory_stream * memory_stream, std::streamsize bufsize, std::streamsize max_to_read = -1);

	~ev_buffered_stream();

	virtual int overflow(int c);

	virtual int underflow();

	virtual int sync();

	void set_mode(openmode mode);

	size_t push_to_sync(char *buffer, std::streamsize bytes);

	openmode get_mode() const;
	virtual void get_prefix(char* buffer, std::streamsize bytes, char *prefix, size_t bytes_ptr);
	virtual void get_suffix(char* buffer, std::streamsize bytes, char *suffix, size_t bytes_ptr);
	void set_prefix_len(size_t bytes);
	void set_suffix_len(size_t bytes);
	void consume_all_of_max_len();
	virtual size_t read_from_source(std::streamsize);
	virtual size_t write_to_sync(char * , std::streamsize);
	int low_readch();

private:
	size_t high_read_from_source(std::streamsize size);

	std::streamsize			_bufsize;
	char*					_p_buffer;
	openmode				_mode;
	chunked_memory_stream*	_memory_stream;
	void*					_nodeptr;
	size_t					_prev_len;
	ssize_t					_max_len;
	size_t					_cum_len;
	char*					_w_buffer;
	int						_prefix_len;
	int						_suffix_len;
	int						_consume_all_of_max_len;
	int						_reading_buf_from_source;

	ev_buffered_stream(const ev_buffered_stream&);
	ev_buffered_stream& operator = (const ev_buffered_stream&);
};

#endif
