#pragma once
#include "IStoragePlugin.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <was/storage_account.h>
#include <was/blob.h>
namespace DIO
{



class MEDIATOOLKIT_API AzureStoragePlugin : virtual public MEDIATOOLKIT_API IStoragePlugin
{
private:
	std::wstring StorageAccountName;
	std::wstring StorageAccountKey;
	std::wstring ConnectionString;
	azure::storage::cloud_storage_account StorageAccount;	
	azure::storage::cloud_blob_client StorageClient;
	const std::wstring FakeFileName = U("$fake.txt");
public:

	/** Dtor
	*
	*/
	virtual ~AzureStoragePlugin()
	{

	}
	AzureStoragePlugin()
	{
		StorageAccountName = L"";
		StorageAccountKey = L"";
		ConnectionString = L"";
	}
	bool SetAzureStorageAccount(const std::wstring& Name, const std::wstring& Key)
	{
		bool result = false;
		try
		{
			StorageAccountName = Name;
			StorageAccountKey = Key;
			ConnectionString = L"DefaultEndpointsProtocol=https;AccountName=" + StorageAccountName + L";AccountKey=" + StorageAccountKey + L";EndpointSuffix=core.windows.net";
			StorageAccount = azure::storage::cloud_storage_account::parse(ConnectionString);
			StorageClient = StorageAccount.create_cloud_blob_client();
			result = true;
		}
		catch (...)
		{
			result = false;
		}
		return result;
	}
	/** Create a reader for this storage's file 
	*
	* \param pathFile the uri of the file - format is implementation specific (c:\folder, ftp://host/folder/file.txt, etc.)
	* \param offset absolute offset from the start of the file, 
	* \param size size of the file to download. .
	* \return the reader. null if failed to create.
	*/
	IRandomAccessStreamReaderPtr CreateReader(
		const std::wstring& pathFile,
		FileProfile* pFileProfile = NULL,
		const ULONGLONG offset = 0,
		const ULONGLONG size = 0,
		IOConfiguration* pUseThisConfiguration = NULL)
	{
		return nullptr;

	};

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
		IOConfiguration* pUseThisConfiguration = NULL)
	{
		return nullptr;
	};
	//Attributes queries
	virtual bool DeleteFile(const std::wstring& fileName)
	{
		bool result = false;
		try
		{
			std::wstring container = GetContainer(fileName);
			std::wstring filePath = GetSubdirectory(fileName);
			// Retrieve a reference to a container.
			azure::storage::cloud_blob_container Container = StorageClient.get_container_reference(container);
			if (Container.exists())
			{
				if (filePath.length() > 0)
				{
					azure::storage::cloud_block_blob Blob = Container.get_block_blob_reference(filePath);
					Blob.delete_blob();
					result = true;
				}
				else
					result = false;

			}
		}
		catch (...)
		{
			result = false;
		}
		return result;
	};
	azure::storage::copy_state GetCopyState(azure::storage::cloud_block_blob& DestBlob)
	{
		azure::storage::copy_state state;
		try
		{
			DestBlob.download_attributes();
			state = DestBlob.copy_state();
		}
		catch (const azure::storage::storage_exception& e)
		{
			ucout << U("Error:") << e.what() << std::endl << U("The copy state could not be retrieved.") << std::endl;
		}
		return state;
	}
	utility::string_t GetCopyStatusDescription(azure::storage::copy_state& state)
	{
		utility::string_t state_description;
		switch (state.status())
		{
		case azure::storage::copy_status::aborted:
			state_description = U("aborted");
			break;
		case azure::storage::copy_status::failed:
			state_description = U("failed");
			break;
		case azure::storage::copy_status::invalid:
			state_description = U("invalid");
			break;
		case azure::storage::copy_status::pending:
			state_description = U("pending");
			break;
		case azure::storage::copy_status::success:
			state_description = U("success");
			break;
		}
		return state_description;
	}

	virtual bool CopyFile(const std::wstring& sourcefileName, const std::wstring& destinationfileName)
	{
		bool result = false;
		try
		{
			std::wstring sourcecontainer = GetContainer(sourcefileName);
			std::wstring sourcefilePath = GetSubdirectory(sourcefileName);
			// Retrieve a reference to a container.
			azure::storage::cloud_blob_container sourceContainer = StorageClient.get_container_reference(sourcecontainer);
			if (sourceContainer.exists())
			{
				if (sourcefilePath.length() > 0)
				{
					azure::storage::cloud_block_blob sourceBlob = sourceContainer.get_block_blob_reference(sourcefilePath);

					std::wstring destinationcontainer = GetContainer(destinationfileName);
					std::wstring destinationfilePath = GetSubdirectory(destinationfileName);
					// Retrieve a reference to a container.
					azure::storage::cloud_blob_container destinationContainer = StorageClient.get_container_reference(destinationcontainer);
					if (destinationContainer.exists())
					{
						if (destinationfilePath.length() > 0)
						{
							azure::storage::cloud_block_blob destinationBlob = destinationContainer.get_block_blob_reference(destinationfilePath);
							utility::string_t copy_id;
							try
							{
							//	ucout << U("Copying blob") << std::endl;
								copy_id = destinationBlob.start_copy(sourceBlob);
							}
							catch (const azure::storage::storage_exception& e)
							{
								ucout << U("Copy Exception ") << e.what() << std::endl;
								result = false;
								return result;
							}

							azure::storage::copy_state state = GetCopyState(destinationBlob);
							utility::string_t state_description = GetCopyStatusDescription(state);
							while (state.status() == azure::storage::copy_status::pending)
							{
								//ucout << U("Copy state ") << state_description << U(" bytes copied ") << state.bytes_copied() << U("/") << state.total_bytes() << std::endl;
								state = GetCopyState(destinationBlob);
								state_description = GetCopyStatusDescription(state);
							}
							if (state.status() == azure::storage::copy_status::success)
								//ucout << U("Copy successful state ") << state_description << U(" bytes copied ") << state.bytes_copied() << std::endl;
								result = true;
							else
								//ucout << U("Copy error state ") << state_description << U(" bytes copied ") << state.bytes_copied() << std::endl;
								result = false;
						}
						else
							result = false;

					}
				}
				else
					result = false;

			}
		}
		catch (...)
		{
			result = false;
		}
		return result;
	};
	virtual bool IsSupported(const std::wstring& fileName) const
	{
		return false;
	};
	virtual bool IsFileExist(const std::wstring& fileName, FileProfile* fileProfile = NULL, bool forceRefresh = false)
	{
		bool result = false;
		try
		{
			std::wstring container = GetContainer(fileName);
			std::wstring filePath = GetSubdirectory(fileName);
			// Retrieve a reference to a container.
			azure::storage::cloud_blob_container Container = StorageClient.get_container_reference(container);
			if (Container.exists())
			{
				if (filePath.length() > 0)
				{
					azure::storage::cloud_block_blob Blob = Container.get_block_blob_reference(filePath);
					result = Blob.exists();
				}
				else
					result = false;

			}
		}
		catch (...)
		{
			result = false;
		}
		return result;
	};
	virtual bool IsDirectoryExist(const std::wstring& directory, FileProfile* fileProfile = NULL, bool forceRefresh = false)
	{
		bool result = false;
		try
		{
			std::wstring container = GetContainer(directory);
			std::wstring subdirectory = GetSubdirectory(directory);
			// Retrieve a reference to a container.
			azure::storage::cloud_blob_container Container = StorageClient.get_container_reference(container);
			if (Container.exists())
			{
				if (subdirectory.length() > 0)
				{
					azure::storage::cloud_block_blob Blob = Container.get_block_blob_reference(subdirectory + U("\\") + FakeFileName);
					result = Blob.exists();
				}
				else
					result = false;

			}
		}
		catch (...)
		{
			result = false;
		}
		return result;
	};
	virtual bool GetSize(const std::wstring& fileName , ULONGLONG &size, FileProfile* fileProfile = NULL, bool forceRefresh = false)
	{
		return false;
	};
	virtual bool IsReadable(const std::wstring& fileName, FileProfile* fileProfile = NULL, bool forceRefresh = false)
	{
		return false;
	};
	virtual bool IsWriteable(const std::wstring& fileName, bool createFile = false, FileProfile* fileProfile = NULL, bool forceRefresh = false)
	{
		return false;
	};
	virtual bool IsReadOnly(const std::wstring& fileName, bool& result, FileProfile* fileProfile = NULL, bool forceRefresh = false)
	{
		return false;
	};
	virtual bool GetLastWriteTime(const std::wstring& fileName, FILETIME& fileTime, FileProfile* fileProfile = NULL, bool forceRefresh = false)
	{
		return false;
	};
	// flecoqui
//	virtual bool GetFileNamesByPattern(const std::wstring& filePattern, list<std::wstring>& fileNames, const std::wstring& wholeDirectory = _T("")) = 0;
	virtual bool GetFileNamesByPattern(const std::wstring& filePattern, std::list<std::wstring>& fileNames, const std::wstring& wholeDirectory = _T(""))
	{
		return false;
	};
	virtual std::wstring GetFileProperty(
		const std::wstring& fileName, 
		const std::wstring& propertyName)
	{
		return false;
	};
	// flecoqui
//	virtual bool GetSubDirectories(const std::wstring& rootDir, list<std::wstring>& subDirectories, int depth = -1) = 0;
	virtual bool GetSubDirectories(const std::wstring& rootDir, std::list<std::wstring>& subDirectories, int depth = -1)
	{
		return false;
	};
	std::wstring GetContainer(const std::wstring& directory)
	{
		std::wstring container = L"";

		try
		{
			std::size_t pos = directory.find_first_of(L"\\/");
			if (pos == 0)
			{
				container = directory.substr(1);
			}
			else
				container = directory;
			pos = container.find_first_of(L"\\/");
			if (pos > 0)
			{
				container = container.substr(0, pos);
			}
		}
		catch (const std::exception& ex)
		{
			std::wcout << U("Error: ") << ex.what() << std::endl;
			container = L"";
		}
		return container;

	}
	std::wstring GetSubdirectory(const std::wstring& directory)
	{
		std::wstring container = L"";
		std::wstring subdirectory = L"";
		try
		{
			std::size_t pos = directory.find_first_of(L"\\/");
			if (pos == 0)
			{
				container = directory.substr(1);
			}
			else
				container = directory;
			pos = container.find_first_of(L"\\/");
			if (pos > 0)
			{
				if(pos+1 < (size_t) container.length())
					subdirectory = container.substr(pos + 1);
			}
		}
		catch (const std::exception& ex)
		{
			std::wcout << U("Error: ") << ex.what() << std::endl;
			subdirectory = L"";
		}
		return subdirectory;

	}
	std::wstring& ReplaceString(std::wstring& s, std::wstring& from, std::wstring& to)
	{
		if (!from.empty())
			for (size_t pos = 0; (pos = s.find(from, pos)) != std::string::npos; pos += to.size())
				s.replace(pos, from.size(), to);
		return s;
	}
	std::wstring GetFilePathFromUrl(std::wstring& directory, const std::wstring& url)
	{
		std::wstring path = L"";
		try
		{
			std::wstring from = L"\\";
			std::wstring to = L"/";

			std::wstring subdirectory = ReplaceString(directory, from, to);
			std::size_t pos = url.find(subdirectory);
			if (pos > 0)
			{
				path = url.substr(pos);
			}
		}
		catch (const std::exception& ex)
		{
			std::wcout << U("Error: ") << ex.what() << std::endl;
			path = L"";
		}
		return path;

	}

	virtual bool CreateDirectoryIfNotExist(const std::wstring& directory)
	{
		bool result = false;
		try
		{
			std::wstring container = GetContainer(directory);
			std::wstring subdirectory = GetSubdirectory(directory);
			if (container.length() > 0)
			{
				// Retrieve a reference to a container.
				azure::storage::cloud_blob_container Container = StorageClient.get_container_reference(container);
				result = Container.create_if_not_exists();
				if (subdirectory.length() > 0)
				{
					azure::storage::cloud_block_blob Blob = Container.get_block_blob_reference(subdirectory + U("\\") + FakeFileName);
					
					std::stringstream oss;
					std::time_t t = std::time(nullptr);
					std::tm tm;
					if (!gmtime_s(&tm, &t))
					{
						oss << std::put_time(&tm, "%c %Z");
						std::string content = oss.str();
						std::wstring wcontent;
						wcontent = wcontent.assign(content.begin(), content.end());
						Blob.upload_text(wcontent);
					}
				}
				result = true;
			}
		}
		catch (const std::exception& ex)
		{
			std::wcout << U("Error: ") << ex.what() << std::endl;
			result = false;
		}
		return result;
	};
	virtual bool CreateFileIfNotExist(const std::wstring& sourcefilename)
	{
		bool result = false;
		try
		{
			std::wstring container = GetContainer(sourcefilename);
			std::wstring filepath = GetSubdirectory(sourcefilename);
			if (container.length() > 0)
			{
				// Retrieve a reference to a container.
				azure::storage::cloud_blob_container Container = StorageClient.get_container_reference(container);
				result = Container.create_if_not_exists();
				if (filepath.length() > 0)
				{
					azure::storage::cloud_block_blob Blob = Container.get_block_blob_reference(filepath);

					std::wstring wcontent;
					wcontent = U("");
					Blob.upload_text(wcontent);
				}
				result = true;
			}
		}
		catch (const std::exception& ex)
		{
			std::wcout << U("Error: ") << ex.what() << std::endl;
			result = false;
		}
		return result;
	};
	bool RemoveSubFolder(azure::storage::cloud_blob_container Container, const std::wstring& folder )
	{
		bool result = false;
		try
		{
			azure::storage::cloud_block_blob Blob = Container.get_block_blob_reference(folder);
			if(Blob.exists())
				Blob.delete_blob();
			result = true;
		}
		catch (...)
		{
			result = false;
		}
		return result;
	};
	bool RemoveFile(azure::storage::cloud_blob_container Container, const std::wstring& file)
	{
		bool result = false;
		try
		{
			azure::storage::cloud_block_blob Blob = Container.get_block_blob_reference(file);
			if (Blob.exists())
				Blob.delete_blob();
			result = true;
		}
		catch (...)
		{
			result = false;
		}
		return result;
	};
	virtual bool RemoveFolder(const std::wstring& directory)
	{
		bool result = false;
		try
		{
			std::wstring container = GetContainer(directory);
			std::wstring subdirectory = GetSubdirectory(directory);
			if (container.length() > 0)
			{
				// Retrieve a reference to a container.
				azure::storage::cloud_blob_container Container = StorageClient.get_container_reference(container);
				if (subdirectory.length() == 0)
					Container.delete_container();
				else
				{
					azure::storage::list_blob_item_iterator end_of_results;
					azure::storage::blob_listing_details::values includes = azure::storage::blob_listing_details::values::all;
					const azure::storage::blob_request_options options;
					azure::storage::operation_context context;
					for (auto it = Container.list_blobs(subdirectory,true, includes,100, options,context); it != end_of_results; ++it)
					{
						if (it->is_blob())
						{
							std::wcout << U("Blob: ") << it->as_blob().uri().primary_uri().to_string() << std::endl;
							std::wstring path = GetFilePathFromUrl(subdirectory, it->as_blob().uri().primary_uri().to_string());
							if(path.length())
								RemoveFile(Container, path);
						}
						else
						{
							std::wcout << U("Directory: ") << it->as_directory().uri().primary_uri().to_string() << std::endl;
						}
					}
					RemoveFile(Container, subdirectory + U("\\") + FakeFileName);
					RemoveSubFolder(Container, subdirectory );
				}
			}
			result = true;
		}
		catch (...)
		{
			result = false;
		}
		return result;
	};

	/** Return the last error message & code
	*
	* \param msg the error message
	* \param code the error code
	* \return true if the last call to the factory completed successfully, false otherwise;
	*/
	virtual bool GetLastErrorMsg(std::wstring& msg, int& code)
	{
		return false;
	};
	virtual bool CopyFiles(const SourceToTargetMap& src2Target, StorageCopyProgressCallBack* progress, bool targetWillBeReadWhileWritten = false)
	{
		return false;
	};
	virtual bool MoveFiles(const SourceToTargetMap& src2Target, StorageCopyProgressCallBack* progress = NULL, bool copyIfMoveFails = false)
	{
		return false;
	};

	//! get attributes of the storage. \see file StorageAttributes.h for supported attributes.
	virtual const StorageAttributes& GetStorageAttributesByURL(const DIO::URL& url)
	{
		return *(new StorageAttributes());
	};
};


} // end of namespace DIO
