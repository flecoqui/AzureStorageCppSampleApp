# Azure Storage Upload Sample App
Azure Storage Upload Sample Application in C++.

Overview
--------
With this application you can upload a local file in a container in Azure Storage.
As parameter of this application you can define the size of the data blocks and the number of threads.


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
      C:\src> cd AzureStorageCppSampleApp\cpp\UploadCpp
      C:\src\AzureStorageCppSampleApp\cpp\UploadCpp> 

**Building the Visual C++ solution**

1. Double-click the Visual Studio 2015 Solution (UploadCpp.sln) file.
4. Press Ctrl+Shift+B, or select **Build** \> **Build Solution**.


Using the application
---------------------
In order to use this application you need to have access to an Azure Storage Account.
You need to know at least the Storage Account Name and the Storage Account Key of your Storage Account.


	UploadCpp.exe    --file <Path to your local file> 
	                 --account <Your Azure Storage Account Name> --key <Your Azure Storage Account Key> 
	                 --container <Your Azure Storage Container> [--blob <Your Azure Storage BlobName>]
	                [--chunksize <ChunkSize Max 4MB default 4MB>]
	                [--threadcount <Number of threads Max 64 default 4>] 
	                [--updateblocklistperiod <Period in ms>] 
	                [--verbose] 



License
-------

Code licensed under the [MIT License](LICENSE.txt).

