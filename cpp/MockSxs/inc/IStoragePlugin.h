#pragma once

#include <string>
#include <list>
#include <functional>
#include <map>
#include <memory>

class Dummy
{
};

#if 0
#include "IRandomAccessStream.h"
#else
typedef Dummy IRandomAccessStream;
typedef Dummy* IRandomAccessStreamReaderPtr;
typedef Dummy* IRandomAccessStreamWriterPtr;
#endif

#include <winnt.h>
#include <tchar.h>
//#include <boost\any.hpp>

#if 0
#include "URL\URL.h"
#else
typedef PVOID URL;
#endif

namespace DIO
{
//class StorageAttributes;
//class IOConfiguration;
//class FileProfile;
typedef Dummy StorageAttributes;
typedef Dummy IOConfiguration;
typedef Dummy FileProfile;

typedef enum
{
	BYTES_UNITS = 0,
	FRAMES_UNITS,
	MSEC_UNITS
}ProgressUnits;

typedef std::function<bool(std::wstring,ULONGLONG,ULONGLONG,ProgressUnits)> StorageCopyProgressCallBack;
typedef std::pair<std::wstring, std::wstring> SourceToTarget;
typedef std::function<bool(ULONGLONG,ULONGLONG,ProgressUnits)> ProgressCallBack;
typedef std::map<URL, URL> SourceToTargetMap;

typedef PVOID URL;

#define MEDIATOOLKIT_API __declspec(dllexport)

class MEDIATOOLKIT_API IStoragePlugin
{
public:

	/** Dtor
	*
	*/
	virtual ~IStoragePlugin()
	{

	}

	/** Create a reader for this storage's file 
	*
	* \param pathFile the uri of the file - format is implementation specific (c:\folder, ftp://host/folder/file.txt, etc.)
	* \param offset absolute offset from the start of the file, 
	* \param size size of the file to download. .
	* \return the reader. null if failed to create.
	*/
	virtual IRandomAccessStreamReaderPtr CreateReader(
		const std::wstring& pathFile, 
		FileProfile* pFileProfile = NULL,
		const ULONGLONG offset = 0, 
		const ULONGLONG size = 0,
		IOConfiguration* pUseThisConfiguration = NULL) = 0;

	/** Create a writer for this storage's file 
	*
	* \param pathFile the uri of the file - format is implementation specific (c:\folder, ftp://host/folder/file.txt, etc.)
	* \param offset absolute offset from the start of the write, TimeCode::zero_ for the start
	* \param estimatedSize size of the file to upload. TimeCode::zero_ for unkonown. can be used as a hint for the storage.
	* \return the reader. null if failed to create.
	*/
	virtual IRandomAccessStreamWriterPtr CreateWriter(
		const std::wstring& pathFile, 
		FileProfile* pFileProfile = NULL,
		const ULONGLONG offset = 0, 
		const ULONGLONG estimatedSize = 0,
		IOConfiguration* pUseThisConfiguration = NULL) = 0;

	//Attributes queries
	virtual bool DeleteFile(const std::wstring& fileName) = 0;
	virtual bool IsSupported(const std::wstring& fileName) const = 0;
	virtual bool IsFileExist(const std::wstring& fileName, FileProfile* fileProfile = NULL, bool forceRefresh = false) = 0;
	virtual bool IsDirectoryExist(const std::wstring& dirName, FileProfile* fileProfile = NULL, bool forceRefresh = false) = 0;
	virtual bool GetSize(const std::wstring& fileName , ULONGLONG &size, FileProfile* fileProfile = NULL, bool forceRefresh = false) = 0;
	virtual bool IsReadable(const std::wstring& fileName, FileProfile* fileProfile = NULL, bool forceRefresh = false) = 0;
	virtual bool IsWriteable(const std::wstring& fileName, bool createFile = false, FileProfile* fileProfile = NULL, bool forceRefresh = false) = 0;
	virtual bool IsReadOnly(const std::wstring& fileName, bool& result, FileProfile* fileProfile = NULL, bool forceRefresh = false) = 0;	
	virtual bool GetLastWriteTime(const std::wstring& fileName, FILETIME& fileTime, FileProfile* fileProfile = NULL, bool forceRefresh = false) = 0;
	virtual bool GetFileNamesByPattern(const std::wstring& filePattern, std::list<std::wstring>& fileNames,const std::wstring& wholeDirectory = _T("")) = 0;
	virtual std::wstring GetFileProperty(
		const std::wstring& fileName, 
		const std::wstring& propertyName) = 0;
	virtual bool GetSubDirectories( const std::wstring& rootDir, std::list<std::wstring>& subDirectories, int depth = -1) = 0;
	virtual bool CreateDirectoryIfNotExist(const std::wstring& directory) = 0;
	virtual bool RemoveFolder(const std::wstring& directory) = 0;

	/** Return the last error message & code
	*
	* \param msg the error message
	* \param code the error code
	* \return true if the last call to the factory completed successfully, false otherwise;
	*/
	virtual bool GetLastErrorMsg(std::wstring& msg, int& code) = 0;
	virtual bool CopyFiles(const SourceToTargetMap& src2Target, StorageCopyProgressCallBack* progress, bool targetWillBeReadWhileWritten = false) = 0;
	virtual bool MoveFiles(const SourceToTargetMap& src2Target, StorageCopyProgressCallBack* progress = NULL, bool copyIfMoveFails = false) = 0;

	//! get attributes of the storage. \see file StorageAttributes.h for supported attributes.
	virtual const StorageAttributes& GetStorageAttributesByURL(const DIO::URL& url) = 0;
};
typedef std::shared_ptr<IStoragePlugin> IStoragePluginPtr;
typedef std::unique_ptr<IStoragePlugin, void (IStoragePlugin*)> IStoragePluginUniquePtr;

} // end of namespace DIO
