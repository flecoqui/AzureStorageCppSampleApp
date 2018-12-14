#ifdef AZURESTORAGEPLUGIN_EXPORTS
#define AZURESTORAGEPLUGIN_API __declspec(dllexport)
#else
#define AZURESTORAGEPLUGIN_API __declspec(dllimport)
#endif

// This class is exported from the AzureStoragePlugin.dll
class AZURESTORAGEPLUGIN_API AzureStoragePlugin : public DIO::IStoragePlugin {
public:
	AzureStoragePlugin(void);

	IRandomAccessStreamReaderPtr CreateReader(
		const std::wstring& pathFile,
		DIO::FileProfile* pFileProfile = NULL,
		const ULONGLONG offset = 0,
		const ULONGLONG size = 0,
		DIO::IOConfiguration* pUseThisConfiguration = NULL);

	IRandomAccessStreamWriterPtr CreateWriter(
		const std::wstring& pathFile,
		DIO::FileProfile* pFileProfile = NULL,
		const ULONGLONG offset = 0,
		const ULONGLONG estimatedSize = 0,
		DIO::IOConfiguration* pUseThisConfiguration = NULL);

	bool DeleteFile(const std::wstring& fileName);
	bool IsSupported(const std::wstring& fileName) const;
	bool IsFileExist(const std::wstring& fileName, DIO::FileProfile* fileProfile = NULL, bool forceRefresh = false);
	bool IsDirectoryExist(const std::wstring& dirName, DIO::FileProfile* fileProfile = NULL, bool forceRefresh = false);
	bool GetSize(const std::wstring& fileName, ULONGLONG &size, DIO::FileProfile* fileProfile = NULL, bool forceRefresh = false);
	bool IsReadable(const std::wstring& fileName, DIO::FileProfile* fileProfile = NULL, bool forceRefresh = false);
	bool IsWriteable(const std::wstring& fileName, bool createFile = false, DIO::FileProfile* fileProfile = NULL, bool forceRefresh = false);
	bool IsReadOnly(const std::wstring& fileName, bool& result, DIO::FileProfile* fileProfile = NULL, bool forceRefresh = false);
	bool GetLastWriteTime(const std::wstring& fileName, FILETIME& fileTime, DIO::FileProfile* fileProfile = NULL, bool forceRefresh = false);
	bool GetFileNamesByPattern(const std::wstring& filePattern, std::list<std::wstring>& fileNames, const std::wstring& wholeDirectory = _T(""));
	std::wstring GetFileProperty(const std::wstring& fileName, const std::wstring& propertyName);
	bool GetSubDirectories(const std::wstring& rootDir, std::list<std::wstring>& subDirectories, int depth = -1);
	bool CreateDirectoryIfNotExist(const std::wstring& directory);
	bool RemoveFolder(const std::wstring& directory);

	bool GetLastErrorMsg(std::wstring& msg, int& code);
	bool CopyFiles(const DIO::SourceToTargetMap& src2Target, DIO::StorageCopyProgressCallBack* progress, bool targetWillBeReadWhileWritten = false);
	bool MoveFiles(const DIO::SourceToTargetMap& src2Target, DIO::StorageCopyProgressCallBack* progress = NULL, bool copyIfMoveFails = false);

	const DIO::StorageAttributes& GetStorageAttributesByURL(const DIO::URL& url);
};
