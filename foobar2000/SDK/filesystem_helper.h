#pragma once

#include <functional>
#include "filesystem.h"

namespace foobar2000_io {
    typedef std::function< void (const char *, t_filestats const & , bool ) > listDirectoryFunc_t;
    void listDirectory( const char * path, abort_callback & aborter, listDirectoryFunc_t func);

#ifdef _WIN32
	pfc::string8 stripParentFolders( const char * inPath );
#endif

	void retryOnSharingViolation( std::function<void () > f, double timeout, abort_callback & a);
	void retryOnSharingViolation( double timeout, abort_callback & a, std::function<void() > f);

	// **** WINDOWS SUCKS ****
	// Special version of retryOnSharingViolation with workarounds for known MoveFile() bugs.
	void retryFileMove( double timeout, abort_callback & a, std::function<void()> f);

	// **** WINDOWS SUCKS ****
	// Special version of retryOnSharingViolation with workarounds for known idiotic problems with folder removal.
	void retryFileDelete( double timeout, abort_callback & a, std::function<void()> f);

	class listDirectoryCallbackImpl : public directory_callback {
	public:
		listDirectoryCallbackImpl() {}
		listDirectoryCallbackImpl( listDirectoryFunc_t f ) : m_func(f) {}
		bool on_entry(filesystem *, abort_callback &, const char * p_url, bool p_is_subdirectory, const t_filestats & p_stats) override {
			m_func(p_url, p_stats, p_is_subdirectory);
			return true;
		}
		listDirectoryFunc_t m_func;
	};

#ifdef _WIN32
	pfc::string8 winGetVolumePath(const char * fb2kPath );
	DWORD winVolumeFlags( const char * fb2kPath );
#endif
}


pfc::string8 file_path_canonical(const char* src);
pfc::string8 file_path_display(const char* src);

namespace fb2k {
    //! Sane replacement for pfc::string_filename_ext(), which isn't safe to use in cross-platform code.
    //! @returns Filename with extension extracted from path.
    pfc::string8 filename_ext( const char * path );
    pfc::string8 filename_ext( const char * path, filesystem::ptr & fs_reuse);
    //! Sane replacement for pfc::string_filename(), which isn't safe to use in cross-platform code
    //! @returns Filename without extension extracted from path.
    pfc::string8 filename( const char * path );
}

class stream_reader_memblock_ref : public stream_reader
{
public:
	template<typename t_array> stream_reader_memblock_ref(const t_array & p_array) : m_data(p_array.get_ptr()), m_data_size(p_array.get_size()), m_pointer(0) {
		pfc::assert_byte_type<typename t_array::t_item>();
	}
	stream_reader_memblock_ref(const void * p_data,t_size p_data_size) : m_data((const unsigned char*)p_data), m_data_size(p_data_size), m_pointer(0) {}
	stream_reader_memblock_ref() : m_data(NULL), m_data_size(0), m_pointer(0) {}
	
	template<typename t_array> void set_data(const t_array & data) {
		pfc::assert_byte_type<typename t_array::t_item>();
		set_data(data.get_ptr(), data.get_size());
	}

	void set_data(const void * data, t_size dataSize) {
		m_pointer = 0;
		m_data = reinterpret_cast<const unsigned char*>(data);
		m_data_size = dataSize;
	}

	t_size read(void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
		p_abort.check();
		t_size delta = pfc::min_t(p_bytes, get_remaining());
		memcpy(p_buffer,m_data+m_pointer,delta);
		m_pointer += delta;
		return delta;
	}
	void read_object(void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
		p_abort.check();
		if (p_bytes > get_remaining()) throw exception_io_data_truncation();
		memcpy(p_buffer,m_data+m_pointer,p_bytes);
		m_pointer += p_bytes;
	}
	t_filesize skip(t_filesize p_bytes,abort_callback & p_abort) {
		p_abort.check();
		t_size remaining = get_remaining();
		if (p_bytes >= remaining) {
			m_pointer = m_data_size; return remaining;
		} else {
			m_pointer += (t_size)p_bytes; return p_bytes;
		}
	}
	void skip_object(t_filesize p_bytes,abort_callback & p_abort) {
		p_abort.check();
		if (p_bytes > get_remaining()) {
			throw exception_io_data_truncation();
		} else {
			m_pointer += (t_size)p_bytes;
		}
	}
	void seek_(t_size offset) {
		PFC_ASSERT( offset <= m_data_size );
		m_pointer = offset;
	}
	const void * get_ptr_() const {return m_data + m_pointer;}
	t_size get_remaining() const {return m_data_size - m_pointer;}
	void reset() {m_pointer = 0;}
private:
	const unsigned char * m_data;
	t_size m_data_size,m_pointer;
};

template<typename buffer_t>
class stream_writer_buffer_simple_t : public stream_writer {
public:
	void write(const void * p_buffer, t_size p_bytes, abort_callback & p_abort) {
		p_abort.check();
		t_size base = m_buffer.get_size();
		if (base + p_bytes < base) throw std::bad_alloc();
		m_buffer.set_size(base + p_bytes);
		memcpy((t_uint8*)m_buffer.get_ptr() + base, p_buffer, p_bytes);
	}
	typedef buffer_t t_buffer; // other classes reference this
	t_buffer m_buffer;
};

typedef stream_writer_buffer_simple_t< pfc::array_t<t_uint8, pfc::alloc_fast> > stream_writer_buffer_simple;

template<class t_storage>
class stream_writer_buffer_append_ref_t : public stream_writer
{
public:
	stream_writer_buffer_append_ref_t(t_storage & p_output) : m_output(p_output) {}
	void write(const void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
		PFC_STATIC_ASSERT( sizeof(m_output[0]) == 1 );
		p_abort.check();
		t_size base = m_output.get_size();
		if (base + p_bytes < base) throw std::bad_alloc();
		m_output.set_size(base + p_bytes);
		memcpy( (t_uint8*) m_output.get_ptr() + base, p_buffer, p_bytes );
	}
private:
	t_storage & m_output;
};

class stream_reader_limited_ref : public stream_reader {
public:
	stream_reader_limited_ref(stream_reader * p_reader,t_filesize p_limit) : m_reader(p_reader), m_remaining(p_limit) {}
	
	t_size read(void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
		if (p_bytes > m_remaining) p_bytes = (t_size)m_remaining;

		t_size done = m_reader->read(p_buffer,p_bytes,p_abort);
		m_remaining -= done;
		return done;
	}

	inline t_filesize get_remaining() const {return m_remaining;}

	t_filesize skip(t_filesize p_bytes,abort_callback & p_abort) {
		if (p_bytes > m_remaining) p_bytes = m_remaining;
		t_filesize done = m_reader->skip(p_bytes,p_abort);
		m_remaining -= done;
		return done;
	}

	void flush_remaining(abort_callback & p_abort) {
		if (m_remaining > 0) skip_object(m_remaining,p_abort);
	}

private:
	stream_reader * m_reader;
	t_filesize m_remaining;
};

class stream_writer_chunk_dwordheader : public stream_writer
{
public:
	stream_writer_chunk_dwordheader(const service_ptr_t<file> & p_writer) : m_writer(p_writer) {}

	void initialize(abort_callback & p_abort) {
		m_headerposition = m_writer->get_position(p_abort);
		m_written = 0;
		m_writer->write_lendian_t((t_uint32)0,p_abort);
	}

	void finalize(abort_callback & p_abort) {
		t_filesize end_offset;
		end_offset = m_writer->get_position(p_abort);
		m_writer->seek(m_headerposition,p_abort);
		m_writer->write_lendian_t(pfc::downcast_guarded<t_uint32>(m_written),p_abort);
		m_writer->seek(end_offset,p_abort);
	}

	void write(const void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
		m_writer->write(p_buffer,p_bytes,p_abort);
		m_written += p_bytes;
	}

private:
	service_ptr_t<file> m_writer;
	t_filesize m_headerposition;
	t_filesize m_written;
};

class stream_writer_chunk : public stream_writer
{
public:
	stream_writer_chunk(stream_writer * p_writer) : m_writer(p_writer), m_buffer_state(0) {}

	void write(const void * p_buffer,t_size p_bytes,abort_callback & p_abort);

	void flush(abort_callback & p_abort);//must be called after writing before object is destroyed
	
private:
	stream_writer * m_writer;
	unsigned m_buffer_state;
	unsigned char m_buffer[255];
};

class stream_reader_chunk : public stream_reader
{
public:
	stream_reader_chunk(stream_reader * p_reader) : m_reader(p_reader), m_buffer_state(0), m_buffer_size(0), m_eof(false) {}

	t_size read(void * p_buffer,t_size p_bytes,abort_callback & p_abort);

	void flush(abort_callback & p_abort);//must be called after reading before object is destroyed

	static void g_skip(stream_reader * p_stream,abort_callback & p_abort);

private:
	stream_reader * m_reader;
	t_size m_buffer_state, m_buffer_size;
	bool m_eof;
	unsigned char m_buffer[255];
};

class stream_reader_dummy : public stream_reader { t_size read(void *,t_size,abort_callback &) override {return 0;} };


















template<bool isBigEndian = false> class stream_reader_formatter {
public:
	stream_reader_formatter(stream_reader & p_stream,abort_callback & p_abort) : m_stream(p_stream), m_abort(p_abort) {}

	template<typename t_int> void read_int(t_int & p_out) {
		if (isBigEndian) m_stream.read_bendian_t(p_out,m_abort);
		else m_stream.read_lendian_t(p_out,m_abort);
	}

	void read_raw(void * p_buffer,t_size p_bytes) {
		m_stream.read_object(p_buffer,p_bytes,m_abort);
	}

	void skip(t_size p_bytes) {m_stream.skip_object(p_bytes,m_abort);}

	template<typename TArray> void read_raw(TArray& data) {
		pfc::assert_byte_type<typename TArray::t_item>();
		read_raw(data.get_ptr(),data.get_size());
	}
	template<typename TArray> void read_byte_block(TArray & data) {
		pfc::assert_byte_type<typename TArray::t_item>();
		t_uint32 size; read_int(size); data.set_size(size);
		read_raw(data);
	}
	template<typename TArray> void read_array(TArray & data) {
		t_uint32 size; *this >> size; data.set_size(size);
		for(t_uint32 walk = 0; walk < size; ++walk) *this >> data[walk];
	}
	void read_string_nullterm( pfc::string_base & ret ) {
		m_stream.read_string_nullterm( ret, m_abort );
	}
    pfc::string8 read_string_nullterm() {
        pfc::string8 ret; this->read_string_nullterm(ret); return ret;
    }
    pfc::string8 read_string() {
        return m_stream.read_string(m_abort);
    }
	stream_reader & m_stream;
	abort_callback & m_abort;
};

template<bool isBigEndian = false> class stream_writer_formatter {
public:
	stream_writer_formatter(stream_writer & p_stream,abort_callback & p_abort) : m_stream(p_stream), m_abort(p_abort) {}

	template<typename t_int> void write_int(t_int p_int) {
		if (isBigEndian) m_stream.write_bendian_t(p_int,m_abort);
		else m_stream.write_lendian_t(p_int,m_abort);
	}

	void write_raw(const void * p_buffer,t_size p_bytes) {
		m_stream.write_object(p_buffer,p_bytes,m_abort);
	}
	template<typename TArray> void write_raw(const TArray& data) {
		pfc::assert_byte_type<typename TArray::t_item>();
		write_raw(data.get_ptr(),data.get_size());
	}

	template<typename TArray> void write_byte_block(const TArray& data) {
		pfc::assert_byte_type<typename TArray::t_item>();
		write_int( pfc::downcast_guarded<t_uint32>(data.get_size()) );
		write_raw( data );
	}
	template<typename TArray> void write_array(const TArray& data) {
		const t_uint32 size = pfc::downcast_guarded<t_uint32>(data.get_size());
		*this << size;
		for(t_uint32 walk = 0; walk < size; ++walk) *this << data[walk];
	}

	void write_string(const char * str) {
		const t_size len = strlen(str);
		*this << pfc::downcast_guarded<t_uint32>(len);
		write_raw(str, len);
	}
	void write_string(const char * str, t_size len_) {
		const t_size len = pfc::strlen_max(str, len_);
		*this << pfc::downcast_guarded<t_uint32>(len);
		write_raw(str, len);
	}
	void write_string_nullterm( const char * str ) {
		this->write_raw( str, strlen(str)+1 );
	}

	stream_writer & m_stream;
	abort_callback & m_abort;
};


#define __DECLARE_INT_OVERLOADS(TYPE)	\
	template<bool isBigEndian> inline stream_reader_formatter<isBigEndian> & operator>>(stream_reader_formatter<isBigEndian> & p_stream,TYPE & p_int) {typename pfc::sized_int_t<sizeof(TYPE)>::t_unsigned temp;p_stream.read_int(temp); p_int = (TYPE) temp; return p_stream;}	\
	template<bool isBigEndian> inline stream_writer_formatter<isBigEndian> & operator<<(stream_writer_formatter<isBigEndian> & p_stream,TYPE p_int) {p_stream.write_int((typename pfc::sized_int_t<sizeof(TYPE)>::t_unsigned)p_int); return p_stream;}

__DECLARE_INT_OVERLOADS(char);
__DECLARE_INT_OVERLOADS(signed char);
__DECLARE_INT_OVERLOADS(unsigned char);
__DECLARE_INT_OVERLOADS(signed short);
__DECLARE_INT_OVERLOADS(unsigned short);

__DECLARE_INT_OVERLOADS(signed int);
__DECLARE_INT_OVERLOADS(unsigned int);

__DECLARE_INT_OVERLOADS(signed long);
__DECLARE_INT_OVERLOADS(unsigned long);

__DECLARE_INT_OVERLOADS(signed long long);
__DECLARE_INT_OVERLOADS(unsigned long long);

__DECLARE_INT_OVERLOADS(wchar_t);


#undef __DECLARE_INT_OVERLOADS

template<typename TVal> class _IsTypeByte {
public:
	enum {value = pfc::is_same_type<TVal,char>::value || pfc::is_same_type<TVal,unsigned char>::value || pfc::is_same_type<TVal,signed char>::value};
};

template<bool isBigEndian,typename TVal,size_t Count> stream_reader_formatter<isBigEndian> & operator>>(stream_reader_formatter<isBigEndian> & p_stream,TVal (& p_array)[Count]) {
	if constexpr (_IsTypeByte<TVal>::value) {
		p_stream.read_raw(p_array,Count);
	} else {
		for(t_size walk = 0; walk < Count; ++walk) p_stream >> p_array[walk];
	}
	return p_stream;
}

template<bool isBigEndian,typename TVal,size_t Count> stream_writer_formatter<isBigEndian> & operator<<(stream_writer_formatter<isBigEndian> & p_stream,TVal const (& p_array)[Count]) {
	if constexpr (_IsTypeByte<TVal>::value) {
		p_stream.write_raw(p_array,Count);
	} else {
		for(t_size walk = 0; walk < Count; ++walk) p_stream << p_array[walk];
	}
	return p_stream;
}

#define FB2K_STREAM_READER_OVERLOAD(type) \
	template<bool isBigEndian> stream_reader_formatter<isBigEndian> & operator>>(stream_reader_formatter<isBigEndian> & stream,type & value)

#define FB2K_STREAM_WRITER_OVERLOAD(type) \
	template<bool isBigEndian> stream_writer_formatter<isBigEndian> & operator<<(stream_writer_formatter<isBigEndian> & stream,const type & value)

FB2K_STREAM_READER_OVERLOAD(GUID) {
	return stream >> value.Data1 >> value.Data2 >> value.Data3 >> value.Data4;
}

FB2K_STREAM_WRITER_OVERLOAD(GUID) {
	return stream << value.Data1 << value.Data2 << value.Data3 << value.Data4;
}

FB2K_STREAM_READER_OVERLOAD(pfc::string) {
	t_uint32 len; stream >> len;
	value = stream.m_stream.read_string_ex(len,stream.m_abort);
	return stream;
}

FB2K_STREAM_WRITER_OVERLOAD(pfc::string) {
	stream << pfc::downcast_guarded<t_uint32>(value.length());
	stream.write_raw(value.ptr(),value.length());
	return stream;
}

FB2K_STREAM_READER_OVERLOAD(pfc::string_base) {
	stream.m_stream.read_string(value, stream.m_abort);
	return stream;
}
FB2K_STREAM_WRITER_OVERLOAD(pfc::string_base) {
	const char * val = value.get_ptr();
	const t_size len = strlen(val);
	stream << pfc::downcast_guarded<t_uint32>(len);
	stream.write_raw(val,len);
	return stream;
}


FB2K_STREAM_WRITER_OVERLOAD(float) {
	union {
		float f; t_uint32 i;
	} u; u.f = value;
	return stream << u.i;
}

FB2K_STREAM_READER_OVERLOAD(float) {
	union { float f; t_uint32 i;} u;
	stream >> u.i; value = u.f;
	return stream;
}

FB2K_STREAM_WRITER_OVERLOAD(double) {
	union {
		double f; t_uint64 i;
	} u; u.f = value;
	return stream << u.i;
}

FB2K_STREAM_READER_OVERLOAD(double) {
	union { double f; t_uint64 i;} u;
	stream >> u.i; value = u.f;
	return stream;
}

FB2K_STREAM_WRITER_OVERLOAD(bool) {
	t_uint8 temp = value ? 1 : 0;
	return stream << temp;
}
FB2K_STREAM_READER_OVERLOAD(bool) {
	t_uint8 temp; stream >> temp; value = temp != 0;
	return stream;
}

template<bool BE = false>
class stream_writer_formatter_simple : public stream_writer_formatter<BE> {
public:
	stream_writer_formatter_simple() : stream_writer_formatter<BE>(_m_stream,fb2k::noAbort), m_buffer(_m_stream.m_buffer) {}

	typedef stream_writer_buffer_simple::t_buffer t_buffer;
	t_buffer & m_buffer;
private:
	stream_writer_buffer_simple _m_stream;
};

template<bool BE = false>
class stream_reader_formatter_simple_ref : public stream_reader_formatter<BE> {
public:
	stream_reader_formatter_simple_ref(const void * source, t_size sourceSize) : stream_reader_formatter<BE>(_m_stream,fb2k::noAbort), _m_stream(source,sourceSize) {}
	template<typename TSource> stream_reader_formatter_simple_ref(const TSource& source) : stream_reader_formatter<BE>(_m_stream,fb2k::noAbort), _m_stream(source) {}
	stream_reader_formatter_simple_ref() : stream_reader_formatter<BE>(_m_stream,fb2k::noAbort) {}

	void set_data(const void * source, t_size sourceSize) {_m_stream.set_data(source,sourceSize);}
	template<typename TSource> void set_data(const TSource & source) {_m_stream.set_data(source);}

	void reset() {_m_stream.reset();}
	t_size get_remaining() {return _m_stream.get_remaining();}

	const void * get_ptr_() const {return _m_stream.get_ptr_();}
private:
	stream_reader_memblock_ref _m_stream;
};

template<bool BE = false>
class stream_reader_formatter_simple : public stream_reader_formatter_simple_ref<BE> {
public:
	stream_reader_formatter_simple() {}
	stream_reader_formatter_simple(const void * source, t_size sourceSize) {set_data(source,sourceSize);}
	template<typename TSource> stream_reader_formatter_simple(const TSource & source) {set_data(source);}
	
	void set_data(const void * source, t_size sourceSize) {
		m_content.set_data_fromptr(reinterpret_cast<const t_uint8*>(source), sourceSize);
		onContentChange();
	}
	template<typename TSource> void set_data(const TSource & source) {
		m_content = source;
		onContentChange();
	}
private:
	void onContentChange() {
		stream_reader_formatter_simple_ref<BE>::set_data(m_content);
	}
	pfc::array_t<t_uint8> m_content;
};






template<bool isBigEndian> class _stream_reader_formatter_translator {
public:
	_stream_reader_formatter_translator(stream_reader_formatter<isBigEndian> & stream) : m_stream(stream) {}
	typedef _stream_reader_formatter_translator<isBigEndian> t_self;
	template<typename t_what> t_self & operator||(t_what & out) {m_stream >> out; return *this;}
private:
	stream_reader_formatter<isBigEndian> & m_stream;
};
template<bool isBigEndian> class _stream_writer_formatter_translator {
public:
	_stream_writer_formatter_translator(stream_writer_formatter<isBigEndian> & stream) : m_stream(stream) {}
	typedef _stream_writer_formatter_translator<isBigEndian> t_self;
	template<typename t_what> t_self & operator||(const t_what & in) {m_stream << in; return *this;}
private:
	stream_writer_formatter<isBigEndian> & m_stream;
};

#define FB2K_STREAM_RECORD_OVERLOAD(type, code) \
	FB2K_STREAM_READER_OVERLOAD(type) {	\
		_stream_reader_formatter_translator<isBigEndian> streamEx(stream);	\
		streamEx || code;	\
		return stream; \
	}	\
	FB2K_STREAM_WRITER_OVERLOAD(type) {	\
		_stream_writer_formatter_translator<isBigEndian> streamEx(stream);	\
		streamEx || code;	\
		return stream;	\
	}




#define FB2K_RETRY_ON_EXCEPTION(OP, ABORT, TIMEOUT, EXCEPTION) \
	{	\
		pfc::lores_timer timer; timer.start();	\
		for(;;) {	\
			try { {OP;} break;	}	\
			catch(const EXCEPTION &) { if (timer.query() > TIMEOUT) throw;}	\
			ABORT.sleep(0.05);	\
		}	\
	}

#define FB2K_RETRY_ON_EXCEPTION2(OP, ABORT, TIMEOUT, EXCEPTION1, EXCEPTION2) \
	{	\
		pfc::lores_timer timer; timer.start();	\
		for(;;) {	\
			try { {OP;} break;	}	\
			catch(const EXCEPTION1 &) { if (timer.query() > TIMEOUT) throw;}	\
			catch(const EXCEPTION2 &) { if (timer.query() > TIMEOUT) throw;}	\
			ABORT.sleep(0.05);	\
		}	\
	}

#define FB2K_RETRY_ON_EXCEPTION3(OP, ABORT, TIMEOUT, EXCEPTION1, EXCEPTION2, EXCEPTION3) \
	{	\
		pfc::lores_timer timer; timer.start();	\
		for(;;) {	\
			try { {OP;} break;	}	\
			catch(const EXCEPTION1 &) { if (timer.query() > TIMEOUT) throw;}	\
			catch(const EXCEPTION2 &) { if (timer.query() > TIMEOUT) throw;}	\
			catch(const EXCEPTION3 &) { if (timer.query() > TIMEOUT) throw;}	\
			ABORT.sleep(0.05);	\
		}	\
	}

#define FB2K_RETRY_ON_SHARING_VIOLATION(OP, ABORT, TIMEOUT) FB2K_RETRY_ON_EXCEPTION(OP, ABORT, TIMEOUT, exception_io_sharing_violation)

// **** WINDOWS SUCKS ****
// File move ops must be retried on all these because you get access-denied when someone is holding open handles to something you're trying to move, or already-exists on something you just told Windows to move away
#define FB2K_RETRY_FILE_MOVE(OP, ABORT, TIMEOUT) FB2K_RETRY_ON_EXCEPTION3(OP, ABORT, TIMEOUT, exception_io_sharing_violation, exception_io_denied, exception_io_already_exists)
	
class fileRestorePositionScope {
public:
	fileRestorePositionScope(file::ptr f, abort_callback & a) : m_file(f), m_abort(a) {
		m_offset = f->get_position(a);
	}
	~fileRestorePositionScope() {
		try {
			if (!m_abort.is_aborting()) m_file->seek(m_offset, m_abort);
		} catch(...) {}
	}
private:
	file::ptr m_file;
	t_filesize m_offset;
	abort_callback & m_abort;
};


//! Debug self-test function for testing a file object implementation, performs various behavior validity checks, random access etc. Output goes to fb2k console.
//! Returns true on success, false on failure (buggy file object implementation).
bool fb2kFileSelfTest(file::ptr f, abort_callback & aborter);
