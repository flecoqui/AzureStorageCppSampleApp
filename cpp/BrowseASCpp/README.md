# Browse Azure Storage Sample App
Browse Azure Storage Sample Application in C++.

Overview
--------
With this application you can search for containers, directories and blobs whose name matches with a filter string.
This application is an extension of the existing Azure Storage API which only supports prefix for the search.
Currently you can only filter the blobs whose name start with a specific string.


Building the application
------------------------

**Installing Vcpkg**

**Prerequisites:**
Windows 10, 8.1, 7
Visual Studio Visual Studio 2015 Update 3 (on Windows)
Git

**Building vcpkg for Windows:**

      C:\> mkdir \src
      C:\> cd \src
      C:\src> git clone https://github.com/Microsoft/vcpkg.git
      C:\src> cd vcpkg
      C:\src\vcpkg> .\bootstrap-vcpkg.bat
      C:\src\vcpkg> .\vcpkg integrate install

**Installing Azure Storage Library with vcpkg:**

To install the Azure Storage Client Library for C++ through Vcpkg, you need Vcpkg installed first. Please follow the instructions(https://github.com/Microsoft/vcpkg#quick-start) to install Vcpkg.
install package with:

      C:\src\vcpkg> .\vcpkg install azure-storage-cpp

**Cloning source code from github:**

      C:\> cd \src
      C:\src> git clone https://github.com/flecoqui/AzureStorageCppSampleApp.git
      C:\src> cd AzureStorageCppSampleApp\cpp\BrowseASCpp
      C:\src\AzureStorageCppSampleApp\cpp\BrowseASCpp> 

**Building the Visual C++ solution**

1. Double-click the Visual Studio 2015 Solution (BrowseASCpp.sln) file.
4. Press Ctrl+Shift+B, or select **Build** \> **Build Solution**.


Using the application
---------------------
In order to use this application you need to have access to an Azure Storage Account.
You need to know at least the Storage Account Name and the Storage Account Key of your Storage Account.


	BrowseASCpp.exe       --account <Your Azure Storage Account Name> --key <Your Azure Storage Account Key> 
		             [--container <Your Azure Storage Container> [--directory <Your Azure Storage Directory>]]
		             [--filter <Your Azure Storage Search Filter>]
		             [--ignorecase]



**List all the containers, directories and blobs in the storage account:**

	BrowseASCpp.exe     --account <Your Azure Storage Account Name> --key <Your Azure Storage Account Key> 
                        --filter *


**List all the directories and blobs in a container of the storage account:**

	BrowseASCpp.exe     --account <Your Azure Storage Account Name> --key <Your Azure Storage Account Key> 
                        --container <Your Azure Storage Container>
                        --filter *

**List all the directories and blobs in a directory in a container of the storage account:**

	BrowseASCpp.exe     --account <Your Azure Storage Account Name> --key <Your Azure Storage Account Key> 
                        --container <Your Azure Storage Container>
                        --directory <Your Azure Storage Directory>
                        --filter *

**List all the directories and blobs including the string "Seattle" in a directory in a container of the storage account:**

	BrowseASCpp.exe     --account <Your Azure Storage Account Name> --key <Your Azure Storage Account Key> 
                        --container <Your Azure Storage Container>
                        --directory <Your Azure Storage Directory>
                        --filter Seattle

**List all the directories and blobs including the string "Seattle" and ignoring case in a directory in a container of the storage account:**

	BrowseASCpp.exe     --account <Your Azure Storage Account Name> --key <Your Azure Storage Account Key> 
                        --container <Your Azure Storage Container>
                        --directory <Your Azure Storage Directory>
                        --filer Seattle
                        --ignorecase

**List all the directories and blobs whose name is ending with ".jpg" and ignoring case in a directory in a container of the storage account:**

	BrowseASCpp.exe     --account <Your Azure Storage Account Name> --key <Your Azure Storage Account Key> 
                        --container <Your Azure Storage Container>
                        --directory <Your Azure Storage Directory>
                        --filter .jpg$
                        --ignorecase

License
-------

Code licensed under the [MIT License](LICENSE.txt).

