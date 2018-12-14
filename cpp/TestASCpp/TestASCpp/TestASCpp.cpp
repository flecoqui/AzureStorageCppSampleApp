// TestASCpp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <was/storage_account.h>
#include <was/blob.h>
#include <cpprest/filestream.h>  
#include <cpprest/containerstream.h> 
#include "AzureStoragePlugin.h"


int main()
{
	//DIO::AzureStoragePlugin* plugin = new DIO::AzureStoragePlugin;
	std::unique_ptr<DIO::AzureStoragePlugin> pAbs(new DIO::AzureStoragePlugin());
	if (pAbs != nullptr)
	{
		bool result = false;
		const std::wstring StorageAccountName = L"YourStorageAccountName";
		const std::wstring StorageAccountKey = L"YourStorageAccountKey";
		result = pAbs->SetAzureStorageAccount(StorageAccountName, StorageAccountKey);
		if (result == true)
		{
			bool bExist = false;
			std::wstring container = L"mycontainer";
			std::wstring subfolder = L"mysubfolder";
			std::wstring blob = L"myblob";
			std::wstring destblob = L"mydestblob";
			std::wstring sourcefile = L"mycontainer/subfolder/subsubfolder/myblob";
			std::wstring destinationfile = L"mycontainer/subfolder/subsubfolder/mydestinationblob";

			
			// Test 1: Create Remove Container
			if (pAbs->IsDirectoryExist(container) == false)
				bExist = pAbs->CreateDirectoryIfNotExist(container);
			else
				bExist = true;
			if(bExist==true)
				bExist = pAbs->RemoveFolder(container);
			if(bExist==true)
				std::wcout << U("Test 1: successful ") << std::endl;
			else
				std::wcout << U("Test 1: not successful ") << std::endl;

			// Sleep 30 seconds 
			// You need 30 seconds to delete a container
			std::wcout << U("Waiting 30 seconds...") << std::endl;
			Sleep(30000);
			// Create container before Test 2
			bExist = pAbs->CreateDirectoryIfNotExist(container);
			// Test 2: Create Remove Container
			if (pAbs->IsDirectoryExist(container) == false)
				bExist = pAbs->CreateDirectoryIfNotExist(container);
			else
				bExist = true;
			if (bExist == true)
				bExist = pAbs->RemoveFolder(container);

			if (bExist == true)
				std::wcout << U("Test 2: successful ") << std::endl;
			else
				std::wcout << U("Test 2: not successful ") << std::endl;

			// You need 30 seconds to delete a container
			std::wcout << U("Waiting 30 seconds...") << std::endl;
			Sleep(30000);
			// Create subfolder
			container = L"mycontainer\\subfolder";
			// Test 3: Create Remove Container
			if (pAbs->IsDirectoryExist(container) == false)
				bExist = pAbs->CreateDirectoryIfNotExist(container);
			else
				bExist = true;
			if (bExist == true)
				bExist = pAbs->RemoveFolder(container);

			if (bExist == true)
				std::wcout << U("Test 3: successful ") << std::endl;
			else
				std::wcout << U("Test 3: not successful ") << std::endl;

			// You need 30 seconds to delete a container
			std::wcout << U("Waiting 30 seconds...") << std::endl;
			Sleep(30000);

			
			container = L"mycontainer/subfolder/subsubfolder";
			// Test 4: Create Remove Container
			if (pAbs->IsDirectoryExist(container) == false)
				bExist = pAbs->CreateDirectoryIfNotExist(container);
			else
				bExist = true;
			if (bExist == true)
				bExist = pAbs->RemoveFolder(container);

			if (bExist == true)
				std::wcout << U("Test 4: successful ") << std::endl;
			else
				std::wcout << U("Test 4: not successful ") << std::endl;
			
			// Test 5

			if (pAbs->IsFileExist(sourcefile) == false)
				bExist = pAbs->CreateFileIfNotExist(sourcefile);
			else
				bExist = true;
			if (bExist == true)
				bExist = pAbs->CopyFile(sourcefile, destinationfile);
			if (bExist == true)
				std::wcout << U("Test 5: successful ") << std::endl;
			else
				std::wcout << U("Test 5: not successful ") << std::endl;


		}
	}

    return 0;
}

