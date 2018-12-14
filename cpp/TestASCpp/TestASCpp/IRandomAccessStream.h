#pragma once
#include <boost/enum.hpp>
// flecoqui
#include <boost/shared_ptr.hpp>

//#include "boost/enum.hpp"

namespace DIO
{
class  IRandomAccessStream
{
public:

	enum SeekMode
	{
		StreamBegin = 0,
		StreamCurrent = 1,
		StreamEnd = 2
	};

	BOOST_ENUM_VALUES(MetaDatorConfig, const char*,
		//! will update the cache that file exist, after a succsufll call to Open()
		(FileCreatedOnOpen)("File Created On Open")  
		//! will update the cache that file exist, after the first read\write 
		(FileCreatedOnFirstTransfer)("File Created On First Transfer")  
		);

public:

	/**
	* Close the stream
	*/
	virtual bool Close() = 0;

	/**
	* Seek the stream to <position> starting at the beginning of the stream
	*/
	virtual bool Seek(ULONGLONG position, SeekMode seekMode = StreamBegin) = 0;

	/** Get the current offset from the start of the stream.
	* This value is updated by calls to Get() and Read() 
	*/
	virtual ULONGLONG GetCurrentPosition() const = 0;

	/* Get the current offset, relative to the original stream
	* If this stream is for instance, a segment in another stream, e.g. a part of a file, this method will return the current offset in the file.
	* In this case GetCurrentPosition() will return the ofset relative to the start of the segment.
	*/
	virtual ULONGLONG GetCurrentPositionInOriginalStream() const = 0;

	/** Get the size of the stream
	*/
	virtual ULONGLONG GetStreamSize() const = 0;

	//locking
	virtual bool Lock(bool exlusiveLock = true, int nLockByteNum = 1, int nLockOffsetInBytes = 0) = 0;
	virtual bool Unlock(int nLockByteNum = 1, int nLockOffsetInBytes = 0 ) = 0;
	
	/** Get the MetaDatorConfig for this reader\writer
	* Tell us info of the file metadata, as:
	* When a file is exist? e.g after Open() or at the first read\write
	*/
	virtual MetaDatorConfig GetMetaDatorConfig() const = 0;

	/**
	* Is the stream is ok ? 
	* Use this to signal IO error like an open a file, or network failure
	*/
	virtual bool IsStreamOK() const = 0;

	/** Get stream id. for debug use only.
	*/
	virtual const std::wstring& GetStreamId() const = 0;

	/** Get the error attributes of the last error
	*/
	virtual const std::wstring& GetLastErrorMsg(int* code = NULL) const = 0;
};

//flecoqui
typedef boost::shared_ptr<IRandomAccessStream> IRandomAccessStreamPtr;

class IRandomAccessStreamReader : public IRandomAccessStream
{
public:
	
	virtual ~IRandomAccessStreamReader() 
	{

	}

	/**
	* Open the stream
	*/
	virtual bool Open() = 0;

	/** Read bytes form stream
	* \param buffer memory to write to. size of 'size'
	* \param size count of bytes to read
	* \return how many bytes where actually read
	*/
	virtual size_t Read(BYTE* buffer, size_t size) = 0;

	/**
	 * Same as above but return true if there was an error in the reading
	 */
	virtual bool ReadEx(BYTE* buffer, size_t size, size_t& numberOfBytesRead) = 0;

	/**
	* Is the stream at its end
	*/
	virtual bool IsEndOfStream() const = 0;
	/**
	* Can read a number of bytes from the stream
	*/
	virtual bool CanReadBytes(size_t howMuch) const = 0;
	
	/** Recheck stream size-return the new size
	*/
	virtual ULONGLONG UpdateStreamSize() = 0;

	/* Does the file grow ? (a.k.a 'while record')
	*/
	virtual bool IsStreamGrow() const = 0;
	//Locking support
	virtual bool Lock( 	bool exlusiveLock = true, int nLockByteNum = 1, int nLockOffsetInBytes = 0) = 0;
	virtual bool Unlock(int nLockByteNum = 1, int nLockOffsetInBytes = 0 ) = 0;
};
//flecoqui
typedef boost::shared_ptr<IRandomAccessStreamReader> IRandomAccessStreamReaderPtr;

class IRandomAccessStreamWriter : public IRandomAccessStream
{
public:
	/**
	 * Open modes of the stream
	 * Defines how the stream should open his underlining resource
	 * All mode opens the stream when the stream position is at 0
	 */
	enum OpenMode
	{
		CreateNew = 1, // create a new resource. return false if already exists or failed to create one
		OpenExisting = 2, // open existing resource. return false if the resource doesn't exist
		TruncateExisting = 4 // open existing resource and truncates it so that its size is 0. return false if the resource doesn't exist

	};

	virtual ~IRandomAccessStreamWriter() {}

	/**
	* Open the stream
	*/
	virtual bool Open(OpenMode openMode = OpenExisting) = 0;


	/**
	 * Write a buffer of size <size> to the stream and return the number of bytes actually written
	 */
	virtual size_t Write(const BYTE* buffer,size_t size) = 0;

	/**
	 * Write a buffer of size <size> and return true if all of the bytes were written
	 */
	virtual bool WriteEx(const BYTE* buffer,size_t size) = 0;

	/**
	 * Flush the stream
	 * If flushAlsoPhisicalDevice = false and this is a stream to a physical device (storage/ftp/etc) flush will be ignored. Mainly to be able to flush internals of decorators
	 */
	virtual bool Flush(bool flushAlsoPhisicalDevice = true) = 0;

	/**
	 * Get alignment constraints for writing. If bigger then 1, the writer must call Write with size aligned to this value or it will fail
	 */
	virtual size_t GetWriteAlignment() const = 0;

	/**
	 * Set the final stream size written.
	 */
	virtual bool SetStreamSize(ULONGLONG streamSize) = 0;

	virtual IRandomAccessStreamReaderPtr GetReader() {return IRandomAccessStreamReaderPtr();}//empty reader
};
//flecoqui
typedef boost::shared_ptr<IRandomAccessStreamWriter> IRandomAccessStreamWriterPtr;


}