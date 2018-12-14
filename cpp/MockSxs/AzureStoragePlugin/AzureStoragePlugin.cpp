// AzureStoragePlugin.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "..\inc\IStoragePlugin.h"
#include "..\inc\AzureStoragePluginAPI.h"
#include "AzureStoragePlugin.h"

using namespace azure::storage;

AzureStoragePlugin::AzureStoragePlugin()
{
    return;
}

IRandomAccessStreamReaderPtr AzureStoragePlugin::CreateReader(
	const std::wstring& pathFile,
	DIO::FileProfile* pFileProfile,
	const ULONGLONG offset,
	const ULONGLONG size,
	DIO::IOConfiguration* pUseThisConfiguration)
{
	return NULL;
}

IRandomAccessStreamWriterPtr AzureStoragePlugin::CreateWriter(
	const std::wstring& pathFile,
	DIO::FileProfile* pFileProfile,
	const ULONGLONG offset,
	const ULONGLONG estimatedSize,
	DIO::IOConfiguration* pUseThisConfiguration)
{
	return NULL;
}

bool AzureStoragePlugin::DeleteFile(const std::wstring& fileName)
{
	return false;
}
bool AzureStoragePlugin::IsSupported(const std::wstring& fileName) const
{
	return false;
}
bool AzureStoragePlugin::IsFileExist(const std::wstring& fileName, DIO::FileProfile* fileProfile, bool forceRefresh)
{
	return false;
}
bool AzureStoragePlugin::IsDirectoryExist(const std::wstring& dirName, DIO::FileProfile* fileProfile, bool forceRefresh)
{
	return false;
}
bool AzureStoragePlugin::GetSize(const std::wstring& fileName, ULONGLONG &size, DIO::FileProfile* fileProfile, bool forceRefresh)
{
	return false;
}
bool AzureStoragePlugin::IsReadable(const std::wstring& fileName, DIO::FileProfile* fileProfile, bool forceRefresh)
{
	return false;
}
bool AzureStoragePlugin::IsWriteable(const std::wstring& fileName, bool createFile, DIO::FileProfile* fileProfile, bool forceRefresh)
{
	return false;
}
bool AzureStoragePlugin::IsReadOnly(const std::wstring& fileName, bool& result, DIO::FileProfile* fileProfile, bool forceRefresh)
{
	return false;
}
bool AzureStoragePlugin::GetLastWriteTime(const std::wstring& fileName, FILETIME& fileTime, DIO::FileProfile* fileProfile, bool forceRefresh)
{
	return false;
}
bool AzureStoragePlugin::GetFileNamesByPattern(const std::wstring& filePattern, std::list<std::wstring>& fileNames, const std::wstring& wholeDirectory)
{
	return false;
}
std::wstring AzureStoragePlugin::GetFileProperty(const std::wstring& fileName, const std::wstring& propertyName)
{
	return NULL;
}
bool AzureStoragePlugin::GetSubDirectories(const std::wstring& rootDir, std::list<std::wstring>& subDirectories, int depth)
{
	return false;
}
bool AzureStoragePlugin::CreateDirectoryIfNotExist(const std::wstring& directory)
{
	return false;
}
bool AzureStoragePlugin::RemoveFolder(const std::wstring& directory)
{
	return false;
}

bool AzureStoragePlugin::GetLastErrorMsg(std::wstring& msg, int& code)
{
	return false;
}
bool AzureStoragePlugin::CopyFiles(const DIO::SourceToTargetMap& src2Target, DIO::StorageCopyProgressCallBack* progress, bool targetWillBeReadWhileWritten)
{
	return false;
}
bool AzureStoragePlugin::MoveFiles(const DIO::SourceToTargetMap& src2Target, DIO::StorageCopyProgressCallBack* progress, bool copyIfMoveFails)
{
	return false;
}

const DIO::StorageAttributes& AzureStoragePlugin::GetStorageAttributesByURL(const DIO::URL& url)
{
	static const DIO::StorageAttributes ret;
	return ret;
}

// Define the connection-string with your values.
const utility::string_t storage_connection_string(U("DefaultEndpointsProtocol=https;AccountName=thunstg1;AccountKey=0Ie/Zr8B6JLkhoBSx9ZqvrR8qxpOHphHKm3SS8/+qSToYz9M8YEKrfSNfPduojpmD+wRzvTR0TXp7AFWpKRHXA=="));

int DoAzureStorageStuff(std::wstring& containerName)
{
	try
	{
		auto dt = utility::datetime::utc_now();
		FILETIME ft;
		ULARGE_INTEGER uli;

		uli.QuadPart = dt.to_interval();
		ft.dwLowDateTime = uli.LowPart;
		ft.dwHighDateTime = uli.HighPart;

		SYSTEMTIME st;
		FileTimeToSystemTime(&ft, &st);

		// Retrieve storage account from connection string.
		//auto storage_account = cloud_storage_account::parse(storage_connection_string);
		auto storage_account = cloud_storage_account_parse(storage_connection_string.c_str());

		// Create the blob client.
		auto blob_client = storage_account.create_cloud_blob_client();

		// Retrieve a reference to a container.
		//auto container = blob_client.get_container_reference(containerName.c_str());
		auto container = cloud_blob_client_get_container_reference(blob_client, containerName.c_str());

		// Create the container if it doesn't already exist.
		container.create_if_not_exists();

		// Make the blob container publicly accessible.
		blob_container_permissions permissions;
		permissions.set_public_access(blob_container_public_access_type::blob);
		container.upload_permissions(permissions);

		// Retrieve reference to a blob named "blob1".
		//auto blockBlob = container.get_block_blob_reference(U("blOb1"));
		auto blockBlob = cloud_blob_container_get_block_blob_reference(container, U("blOb1"));

		// Create or overwrite the "blob1" blob with contents from a local file.
		concurrency::streams::istream input_stream = concurrency::streams::file_stream<uint8_t>::open_istream(U("..\\DataFile.txt")).get();
		blockBlob.upload_from_stream(input_stream);
		input_stream.close().wait();

		// Create or overwrite the "blob2" and "blob3" blobs with contents from text.
		// Retrieve a reference to a blob named "blob2".
		//auto blob2 = container.get_block_blob_reference(U("blOb2"));
		//blob2.upload_text(U("more text"));
		auto blob2 = cloud_blob_container_get_block_blob_reference(container, U("blOb2"));
		cloud_block_blob_upload_text(blob2, U("more text"));

		// Retrieve a reference to a blob named "blob3".
		//auto blob3 = container.get_block_blob_reference(U("dIr/subDir/blOb3"));
		//blob3.upload_text(U("other text"));
		auto blob3 = cloud_blob_container_get_block_blob_reference(container, U("dIr/subDir/blOb3"));
		cloud_block_blob_upload_text(blob3, U("other text"));

	} catch (const std::exception& e) {
		DebugOut(L"Exception : %S", e.what());
	}
	return 0;
}

AZURESTORAGEPLUGIN_API DIO::IStoragePluginPtr CreateAzureStoragePlugin(void)
{
	auto str = std::wstring((wchar_t *)L"FooBar");
	TestStringOut(&str);
	printf("DoIt (from plugin) = %d\n", DoIt(17));

	std::wstring containerName = L"testcontainer";
	DoAzureStorageStuff(containerName);
	return std::make_shared<AzureStoragePlugin>();
}
