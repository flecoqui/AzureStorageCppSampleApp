//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************
// BrowseASCpp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "stdafx.h"


extern bool Match(const std::wstring& text, const std::wstring& filter, bool ignoreCase);

bool ParseCommandLine(int argc, wchar_t* argv[], 
	std::wstring& AzureStorageAccountName,
	std::wstring& AzureStorageAccountKey,
	std::wstring& AzureStorageContainer,
	std::wstring& AzureStoragedirectory,
	std::wstring& AzureStorageFilter,
	bool& bIgnoreCase
	)
{
	bool success = false;
	bIgnoreCase = false;
	while (--argc != 0)
	{

		std::wstring option = *++argv;
		if (option[0] == '-')
		{
			if (option == L"--account")
			{
				if (--argc != 0)
					AzureStorageAccountName = *++argv;
			}
			else if (option == L"--key")
			{
				if (--argc != 0)
					AzureStorageAccountKey = *++argv;
			}
			else if (option == L"--container")
			{
				if (--argc != 0)
					AzureStorageContainer = *++argv;
			}
			else if (option == L"--directory")
			{
				if (--argc != 0)
					AzureStoragedirectory = *++argv;
			}
			else if (option == L"--filter")
			{
				if (--argc != 0)
					AzureStorageFilter = *++argv;
			}
			else if (option == L"--ignorecase")
			{
				bIgnoreCase = true;
			}
			else
				return success;
		}
	}
	if ((AzureStorageAccountName.length() > 0) &&
		(AzureStorageAccountKey.length() > 0))
		success = true;
	return success;
}
bool  GetAzureStorageClient(const wchar_t* Name, const wchar_t* Key, azure::storage::cloud_blob_client&  StorageClient)
{
	bool result = false;
	try
	{
		std::wstring ConnectionString = L"DefaultEndpointsProtocol=https;AccountName=";
		ConnectionString += Name;
		ConnectionString += L";AccountKey=";
		ConnectionString += Key;
		ConnectionString += L";EndpointSuffix=core.windows.net";
		azure::storage::cloud_storage_account StorageAccount = azure::storage::cloud_storage_account::parse(ConnectionString);
		StorageClient = StorageAccount.create_cloud_blob_client();
		result = true;
	}
	catch (...)
	{
		result = false;
	}
	return result;
}


bool IsHexChar(const WCHAR _char)
{
	return ((_char == L'A') ||
		(_char == L'B') ||
		(_char == L'C') ||
		(_char == L'D') ||
		(_char == L'E') ||
		(_char == L'F') ||
		iswalnum(_char));
}

std::wstring UriDecode(const std::wstring& _encodedStr)
{
	std::string charStr;

	for (size_t i = 0; i < _encodedStr.length(); ++i)
	{
		if ((_encodedStr[i] == L'%') && (IsHexChar(_encodedStr[i + 1])) && (IsHexChar(_encodedStr[i + 2])))
		{
			std::wstring hexCodeStr = L"0x";
			hexCodeStr += _encodedStr[i + 1];
			hexCodeStr += _encodedStr[i + 2];

			unsigned int hexCharCode;
			std::wstringstream ss;
			ss << std::hex << hexCodeStr;
			ss >> hexCharCode;

			charStr += static_cast<char>(hexCharCode);

			i += 2;
		}
		else if (_encodedStr[i] == L'+')
			charStr += L' ';
		else
			charStr += (char) _encodedStr[i];
	}

	WCHAR decodedStr[INTERNET_MAX_URL_LENGTH];
	MultiByteToWideChar(CP_UTF8, 0, charStr.c_str(), -1, decodedStr, sizeof(decodedStr));

	return decodedStr;
}

int  ProcessCloudDirectory(azure::storage::cloud_blob_client&  StorageClient, azure::storage::cloud_blob_container&  Container, azure::storage::cloud_blob_directory& Directory, const wchar_t* filter, bool ignoreCase = true)
{
	int Counter = 0;
	try
	{
		if (Container.is_valid())
		{
			if (Directory.is_valid())
			{
				// Search in the whole Directory 
				// 
				azure::storage::list_blob_item_iterator end_of_results;
				for (auto it = Directory.list_blobs(); it != end_of_results; ++it)
				{
					if (it->is_blob())
					{
						utility::string_t s = it->as_blob().name();
						
						std::wstring ws(s.begin(),s.end());
						if (Match(ws.c_str(), filter, ignoreCase))
						{
							ucout << "BLOB:      " << UriDecode(it->as_blob().uri().path()) << std::endl;
							Counter++;
						}
					}
					else
					{
						azure::storage::cloud_blob_directory SubDirectory = it->as_directory();
						if (Match(SubDirectory.uri().path().c_str(), filter, ignoreCase))
						{
							ucout << "DIRECTORY: " << UriDecode(SubDirectory.uri().path()) << std::endl;
							Counter++;
						}
						Counter += ProcessCloudDirectory(StorageClient, Container, SubDirectory, filter, ignoreCase);
					}
				}
			}
			else
			{
				// Search in the whole Container 
				// 
				azure::storage::list_blob_item_iterator end_of_results;
				for (auto it = Container.list_blobs(); it != end_of_results; ++it)
				{
					if (it->is_blob())
					{
						utility::string_t s = it->as_blob().name();
						std::wstring ws(s.begin(), s.end());
						if (Match(ws.c_str(), filter, ignoreCase))
						{
							ucout << "BLOB:      " << UriDecode(it->as_blob().uri().path()) << std::endl;
							Counter++;
						}
					}
					else
					{
						azure::storage::cloud_blob_directory SubDirectory = it->as_directory();
						if (Match(SubDirectory.uri().path().c_str(), filter, ignoreCase))
						{
							ucout << "DIRECTORY: " << UriDecode(SubDirectory.uri().path()) << std::endl;
							Counter++;
						}

						Counter += ProcessCloudDirectory(StorageClient, Container, SubDirectory, filter, ignoreCase);
					}
				}
			}
		}

	}
	catch (...)
	{

	}
	return Counter;
}
int  ProcessCloudContainer(azure::storage::cloud_blob_client&  StorageClient, azure::storage::cloud_blob_container&  Container, azure::storage::cloud_blob_directory& Directory, const wchar_t* filter, bool ignoreCase = true)
{
	int Counter = 0;
	try
	{
		if (Container.is_valid()) {
			if (Match(Container.uri().path().c_str(), filter, ignoreCase))
			{
				ucout << "CONTAINER: " << UriDecode(Container.uri().path()) << std::endl;
				Counter++;
			}
			if (Directory.is_valid())
			{
				if (Match(Directory.uri().path().c_str(), filter, ignoreCase))
				{
					ucout << "DIRECTORY: " << UriDecode(Directory.uri().path()) << std::endl;
					Counter++;
				}
			}
			Counter += ProcessCloudDirectory(StorageClient, Container, Directory, filter, ignoreCase);
		}
		else
		{
			// Search in the whole Storage Account 
			// 
			azure::storage::container_result_iterator end_of_results;
			for (auto it = StorageClient.list_containers(); it != end_of_results; ++it)
			{
				if (it->is_valid())
				{
					if (Match(it->uri().path().c_str(), filter, ignoreCase))
					{
						ucout << "CONTAINER: " << UriDecode(it->uri().path()) << std::endl;
						Counter++;
					}
					if (Directory.is_valid())
					{
						if (Match(Directory.uri().path().c_str(), filter, ignoreCase))
						{
							ucout << "DIRECTORY: " << UriDecode(Directory.uri().path()) << std::endl;
							Counter++;
						}
					}
					Counter += ProcessCloudDirectory(StorageClient, *it, Directory, filter, ignoreCase);
				}
			}
		}
	}
	catch (...)
	{

	}
	return Counter;
}
bool IsDirectoryExists(azure::storage::cloud_blob_directory Directory)
{
	try
	{
		azure::storage::cloud_blob_directory Parent = Directory.get_parent_reference();
		if (!Parent.is_valid())
		{
			azure::storage::list_blob_item_iterator end_of_results;
			for (auto it = Directory.container().list_blobs(); it != end_of_results; ++it)
			{
				if (!it->is_blob())
				{

					if (it->as_directory().uri().path() == Directory.uri().path())
						return true;
				}
			}
			return false;
		}
		else
		{
			azure::storage::list_blob_item_iterator end_of_results;
			for (auto it = Parent.list_blobs(); it != end_of_results; ++it)
			{
				if (!it->is_blob())
				{

					if (it->as_directory().uri().path() == Directory.uri().path())
						return true;
				}
			}
		}
	}
	catch (...)
	{
		return false;
	}
	return false;
}
int _tmain(int argc, wchar_t* argv[])
{
	std::wstring AzureStorageAccountName = U("");
	std::wstring AzureStorageAccountKey = U("");
	std::wstring AzureStorageContainer = U("");
	std::wstring AzureStorageDirectory = U("");
	std::wstring AzureStorageFilter = U("*");
	azure::storage::cloud_storage_account StorageAccount;
	azure::storage::cloud_blob_client StorageClient;
	bool bIgnoreCase;
	_setmode(_fileno(stdout), _O_U16TEXT);

	bool result = ParseCommandLine(argc, argv, AzureStorageAccountName, AzureStorageAccountKey, AzureStorageContainer, AzureStorageDirectory, AzureStorageFilter, bIgnoreCase);
	if (result == false)
	{
		ucout << "Azure Storage Browse Command line tool: syntax error" << std::endl;
		ucout << "Syntax:" << std::endl;
		ucout << "BrowseAS.exe --account \"<Your Azure Storage Account Name>\" --key \"<Your Azure Storage Account Key>\" " << std::endl;
		ucout << "            [--container \"<Your Azure Storage Container>\" [--directory \"<Your Azure Storage Directory>\"]]" << std::endl;
		ucout << "            [--filter \"<Your Azure Storage Search Filter>\"]" << std::endl;
		ucout << "            [--ignorecase]" << std::endl;
		return 0;
	}

	if(GetAzureStorageClient(AzureStorageAccountName.c_str(), AzureStorageAccountKey.c_str(), StorageClient)==true)
	{
		azure::storage::cloud_blob_container Container;
		azure::storage::cloud_blob_directory Directory;
		ucout << "Connected to Storage Account: " << AzureStorageAccountName.c_str() <<  std::endl;
		if (AzureStorageContainer.length() > 0)
		{
			Container = StorageClient.get_container_reference(AzureStorageContainer);
			if (Container.exists())
			{
				// Search in the container 
				if (AzureStorageDirectory.length() > 0)
				{
					Directory = Container.get_directory_reference(AzureStorageDirectory );
					if (!IsDirectoryExists(Directory))
					{
						ucout << "Azure Storage Directory doesn't exists or is empty: " << AzureStorageDirectory.c_str() << std::endl;
						return 0;
					}
				}
			}
			else
			{
				ucout << "Azure Storage Container not exists: " << AzureStorageContainer.c_str() << std::endl;
				return 0;
			}

		}		
		int Counter = 0;
		ucout << "Searching in the Storage Account ..." <<std::endl;
		Counter = ProcessCloudContainer(StorageClient, Container, Directory, AzureStorageFilter.c_str(), bIgnoreCase);
		ucout << Counter << " container(s),Directory(s), blob(s) found for filter: " << AzureStorageFilter.c_str() << std::endl;
	}
	return 1;
}

