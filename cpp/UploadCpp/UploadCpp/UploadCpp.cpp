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
// UploadCpp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "stdafx.h"

void threadproc(int threadid, void *ptr);
/////////////////////////////////////////////////////////////////////////////
// class that holds one I/O read & upload piece
class FileChunk
{
public:
	int id;                                             // seq id of the chunk to read
	unsigned long startpos;                             // offset in file where to start reading
	unsigned long length;                               // length of chunk to read from file
	int threadid;                                       // marked by the thread that pulls the piece from the queue
	bool completed;                                     // marked by the thread when completed
	bool committed;
	unsigned long bytesread;                            // actual bytes read from file
	float seconds;                                      // time it took to send this chunk to Azure Storage
	utility::string_t block_id;                         // BlockId for Azure

public:
	FileChunk(int id, unsigned long startpos, unsigned long length)
	{
		this->id = id;
		this->startpos = startpos;
		this->length = length;
		this->bytesread = 0;
		this->completed = false;
		this->committed = false;
		this->threadid = 0;
		this->seconds = (float)0;
	}
};
/////////////////////////////////////////////////////////////////////////////
//
class BlockUpload
{
private:
	int countThreads;                                       // how many threads to use for parallell upload
	unsigned long chunkSize;                                // size in bytes to send as chunks
	bool updateblocklist;                                   // update block list after each block copy
	std::wstring filename;                                  // local file to upload
	std::wstring blobName;                                  // name of the blob in Azure
	std::list<FileChunk*> chunkl;							// list of chunks that we need to post-process each chunk after being uploaded
	std::queue<FileChunk*> queueChunks;                     // queue holding each chunk to upload that the threads pull from 
	azure::storage::cloud_storage_account storage_account;  // Azure Storage Account object
	azure::storage::cloud_blob_client blob_client;          // Azure Storage client object
	azure::storage::cloud_blob_container container;         // Azure Storage Container object

	azure::storage::blob_request_options options;			// blob request option
	azure::storage::access_condition condition;				// access condition 
	azure::storage::operation_context context;				// operation context

	std::mutex mtx_log;										// mutex used for logs

	unsigned long updateblocklistperiod;					// Update block list period in ms
	float lastupdateblocklisttime;							// time of the last block list update

	bool requestexclusiveaccess;							// request exclusive access to send the block list
	int threadtostop;										// remaining number of threads to stop
	bool block_list_sent;									// block list sent flag

	

	std::mutex mtx_block_list_sent;							// mutex for critical section used to when block list sent
	std::condition_variable cv_block_list_sent;				// condition variable for critical section  used to when block list sent
	std::mutex mtx_all_threads_waiting;						// mutex for critical section used when all thread are on pause
	std::condition_variable cv_all_threads_waiting;			// condition variable for critical section  when all thread are on pause


public:
	float elapsed_secs;                                     // how long in seconds the upload took
	unsigned long total_bytes;                              // bytes uploaded
	bool verbose;											// verbose

public:
	BlockUpload(int threads, unsigned long chunkSize, unsigned long updateblocklistperiod, bool verbose)
	{
		this->countThreads = threads;
		this->chunkSize = chunkSize;
		this->updateblocklistperiod = updateblocklistperiod;
		this->verbose = verbose;
		this->lastupdateblocklisttime = (float) clock();
		options.set_parallelism_factor(threads);
		//options.set_stream_write_size_in_bytes(chunkSize);
		condition = azure::storage::access_condition::generate_empty_condition();
		total_bytes = 0;
		elapsed_secs = (float)0;
		requestexclusiveaccess = false;
		threadtostop = 0;
		block_list_sent = false;
	}
	void Log(const char* message)
	{
		std::unique_lock<std::mutex> lck(mtx_log);
		ucout << message ;
	}
	/////////////////////////////////////////////////////////////////////////////
	// hook up the Azure Storage stuff based on account name, key and container name
	void ConnectToAzureStorage(std::wstring storageAccountName, std::wstring storageAccessKey, std::wstring containerName)
	{
		std::wstring connstr = U( "DefaultEndpointsProtocol=https;AccountName=") + storageAccountName + U(";AccountKey=" )+ storageAccessKey;
		utility::string_t storage_connection_string(connstr.begin(), connstr.end());
		utility::string_t blobContainer(containerName.begin(), containerName.end());
		ConnectToAzureStorage(storage_connection_string, blobContainer);
	}
	/////////////////////////////////////////////////////////////////////////////
	// hook up the Azure Storage stuff
	void ConnectToAzureStorage(utility::string_t connection_string, utility::string_t containerName)
	{
		storage_account = azure::storage::cloud_storage_account::parse(connection_string);
		blob_client = storage_account.create_cloud_blob_client();
		container = blob_client.get_container_reference(containerName);
		container.create_if_not_exists();
	}
	/////////////////////////////////////////////////////////////////////////////
	// upload local file - blob will have same name as local file
	bool UploadFile(std::wstring filename)
	{
		size_t found = filename.find_last_of(U("/\\"));
		//std::string folder = filename.substr(0, found);
		std::wstring blobFilename = filename.substr(found + 1);
		return UploadFile(filename, blobFilename);
	}
	/////////////////////////////////////////////////////////////////////////////
	// upload local file and naming the blob
	bool UploadFile(std::wstring filename, std::wstring blobFilename)
	{
		// get file size
		std::wifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			return false;
		}

		// how large is the file?
		std::streampos size = file.tellg();
		file.close();

		// how many chunks?
		unsigned long chunks = (unsigned long)size / this->chunkSize + 1;
		unsigned long remaining = (unsigned long)size;
		unsigned long chunksread = 0;
		unsigned long currpos = 0;


		// create chunks and push them on a queue
		while (remaining > 0)
		{
			chunksread++;
			long toread = remaining > this->chunkSize ? this->chunkSize : remaining;
			FileChunk *fc = new FileChunk(chunksread, (unsigned long)currpos, (unsigned long)toread);
			chunkl.push_back(fc);
			this->queueChunks.push(fc);
			remaining -= toread;
			currpos += toread;
		}

		this->filename = filename;
		blobName.assign(blobFilename.begin(), blobFilename.end());
		azure::storage::cloud_block_blob blob1 = container.get_block_blob_reference(blobName);

		// start the timer for how fast we process the file
		unsigned t0 = clock();

		// create threads that process tasks in the queue
		std::list<std::thread*> vt;
		for (int n = 1; n <= countThreads; n++)
		{
			std::thread *t1 = new std::thread(threadproc, n, this);
			vt.push_back(t1);
		}

		// wait for all threads to complete
		std::list<std::thread*>::iterator itt;
		for (itt = vt.begin(); itt != vt.end(); ++itt)
		{
			(*itt)->join();
		}

		// stop the timer
		unsigned elapsed = clock() - t0;

		// create the block list vector from results
		this->total_bytes = 0;
		std::vector<azure::storage::block_list_item> vbi;
		std::list<FileChunk*>::iterator it;
		for (it = chunkl.begin(); it != chunkl.end(); ++it)
		{
			azure::storage::block_list_item *bli = new azure::storage::block_list_item((*it)->block_id);
			vbi.push_back(*bli);
			this->total_bytes += (*it)->bytesread;
			delete (*it);
		}

		// commit the block list items to Azure Storage
		blob1.upload_block_list(vbi);

		this->elapsed_secs = (float)elapsed / (float)CLOCKS_PER_SEC;

		return true;
	} 
	std::vector<azure::storage::block_list_item> GetCurrentBlockList(std::list<FileChunk*> chunklist)
	{
		this->total_bytes = 0;
		std::vector<azure::storage::block_list_item> vbi;
		std::list<FileChunk*>::iterator it;
		bool bnew = false;
		for (it = chunklist.begin(); it != chunklist.end(); ++it)
		{
			if ((*it)->completed)
			{
				azure::storage::block_list_item *bli = new azure::storage::block_list_item((*it)->block_id);
				vbi.push_back(*bli);
				if((*it)->committed == false)
				{
					bnew = true;
				}
			}
			else
				break;
		}
		if(bnew == false)
			vbi.clear();
		return vbi;
	}
	bool TimeToUpdateBlockList()
	{
		bool result = false;
		if (this->updateblocklistperiod > 0)
		{
			float t = (float) clock();
			unsigned long tms = (unsigned long) (((float)(t - this->lastupdateblocklisttime) * 1000) / (float)CLOCKS_PER_SEC);
			if(tms > this->updateblocklistperiod)
			{
				this->lastupdateblocklisttime = t;
				result = true;			
			}
		}
		return result;
	}
	//
	/////////////////////////////////////////////////////////////////////////////
	// Wait All Thread On Pause Before Sending Block List
	void WaitAllThreadOnPauseBeforeSendingBlockList()
	{
		if (this->queueChunks.empty())
			return;
		std::unique_lock<std::mutex> lck_all_threads_waiting(mtx_all_threads_waiting);
		while (this->threadtostop) { cv_all_threads_waiting.wait(lck_all_threads_waiting); }
	}
	/////////////////////////////////////////////////////////////////////////////
	// Notify All Thread On Pause Before Sending Block List
	void NotifyAllThreadOnPauseBeforeSendingBlockList()
	{
		std::unique_lock<std::mutex> lck_all_threads_waiting(mtx_all_threads_waiting);
		cv_all_threads_waiting.notify_all();
	}
	/////////////////////////////////////////////////////////////////////////////
	// Wait Block List Sent
	void WaitBlockListSent()
	{
		if (this->queueChunks.empty())
			return;
		std::unique_lock<std::mutex> lck_block_list_sent(mtx_block_list_sent);
		while (!this->block_list_sent) { cv_block_list_sent.wait(lck_block_list_sent); }
	}
	/////////////////////////////////////////////////////////////////////////////
	// Notify Block List Sent
	void NotifyBlockListSent()
	{
		std::unique_lock<std::mutex> lck_block_list_sent(mtx_block_list_sent);
		cv_block_list_sent.notify_all();
	}
	//
	/////////////////////////////////////////////////////////////////////////////
	// processing that takes part in a separate thread 
	void ThreadProc(int threadid)
	{
		
		std::ifstream file(this->filename, std::ios::in | std::ios::binary | std::ios::ate);
		if (file.is_open())
		{
			azure::storage::cloud_block_blob blob = this->container.get_block_blob_reference(this->blobName);

			std::vector<uint8_t> buffer(this->chunkSize);
			// get the next file I/O task from hte queue and read that chunk
			while (!this->queueChunks.empty())
			{
				if ((requestexclusiveaccess == true)&&(this->countThreads > 1))
				{
					this->threadtostop--;
					

					if (this->threadtostop == 0)
					{
						NotifyAllThreadOnPauseBeforeSendingBlockList();
					}
					if (this->verbose)
					{
						std::ostringstream oss;
						oss << "Waiting block list sent in thread: " << threadid << std::endl;
						this->Log(oss.str().c_str());
					}
					WaitBlockListSent();
					if (this->verbose)
					{
						std::ostringstream oss;
						oss << "Waiting block list sent in thread: " << threadid << std::endl;
						this->Log(oss.str().c_str());
					}
					if (this->queueChunks.empty())
						break;
				}
				
				FileChunk *fc = (FileChunk*)(this->queueChunks.front());
				this->queueChunks.pop();

				if (fc->length != this->chunkSize)
					buffer.resize(fc->length);

				// read the specified chunk from the file
				file.seekg(fc->startpos, std::ios::beg);
				file.read((char*)&buffer[0], fc->length);
				fc->bytesread = (unsigned long)file.gcount();

				// create Azure Block ID value
				fc->block_id = utility::conversions::to_base64(fc->id);
				auto stream = concurrency::streams::bytestream::open_istream(buffer);
				utility::string_t md5 = _XPLATSTR("");

				unsigned long t0 = clock();

				if (this->verbose)
				{
					std::ostringstream oss;
					oss << "blob.upload_block in thread: " << threadid << std::endl;
					this->Log(oss.str().c_str());
				}
				blob.upload_block(fc->block_id, stream, md5, condition, options, context);
				fc->threadid = threadid;
				fc->completed = true;
				if (this->verbose)
				{
					std::ostringstream oss;
					oss << "blob.upload_block done in thread: " << threadid << std::endl;
					this->Log(oss.str().c_str());
				}

				fc->seconds = (float)(clock() - t0) / (float)CLOCKS_PER_SEC;

				if ((threadid == 1)&&(!this->queueChunks.empty())&& (TimeToUpdateBlockList()) )
				{
					if (this->countThreads > 1)
					{
						this->block_list_sent = false;
						this->requestexclusiveaccess = true;
						this->threadtostop = this->countThreads - 1;
						if (this->verbose)
						{
							std::ostringstream oss;
							oss << "Waiting for all other threads pause in thread : " << threadid << std::endl;
							this->Log(oss.str().c_str());
						}
						WaitAllThreadOnPauseBeforeSendingBlockList();
						if (this->verbose)
						{
							std::ostringstream oss;
							oss << "Waiting for all other threads pause done in thread : " << threadid << std::endl;
							this->Log(oss.str().c_str());
						}
					}
					try
					{
						if (this->verbose)
						{
							std::ostringstream oss;
							oss << "blob.upload_block_list in thread: " << threadid << std::endl;
							this->Log(oss.str().c_str());
						}

						std::vector<azure::storage::block_list_item> vbli = GetCurrentBlockList(this->chunkl);	
						if (vbli.size() > 0)
						{
							
							std::vector<azure::storage::block_list_item>::iterator vbit;
							
							blob.upload_block_list(vbli, condition, options, context);
							for (vbit = vbli.begin(); vbit != vbli.end(); ++vbit)
							{
								std::list<FileChunk*>::iterator it;
								for (it = this->chunkl.begin(); it != this->chunkl.end(); ++it)
								{
									if ((*it)->block_id == vbit->id())
									{
										(*it)->committed = true;
										break;
									}
								}
							}
						}
						if (this->verbose)
						{
							std::ostringstream oss;
							oss << "blob.upload_block_list done in thread: " << threadid << std::endl;
							this->Log(oss.str().c_str());
						}

					}
					catch (std::exception& e)
					{
						ucout << "Azure Storage Upload Block List - Exception: " << e.what() << std::endl;
						//lck.unlock();
					}

					if (this->countThreads > 1)
					{
						this->requestexclusiveaccess = false;
						this->block_list_sent = true;
						NotifyBlockListSent();
					}
				}
			}
			file.close();
		}
	}
};
/////////////////////////////////////////////////////////////////////////////
// thread entry point 
void threadproc(int threadid, void *ptr)
{
	// return back into the C++ class
	((BlockUpload*)ptr)->ThreadProc(threadid);
}
/////////////////////////////////////////////////////////////////////////////
// split path into folder and filename
void splitpath(const std::wstring& str, std::wstring& folder, std::wstring& filename)
{
	size_t found;
	found = str.find_last_of(U("/\\"));
	folder = str.substr(0, found);
	filename = str.substr(found + 1);
}
/////////////////////////////////////////////////////////////////////////////
// lookup a command line argument and return its index
int find_arg(const char* param, int argc, char* argv[])
{
	for (int n = 0; n <argc; n++)
	{
		if (!strcmp(param, argv[n]))
			return n;
	}
	return -1;
}

bool ParseCommandLine(int argc, wchar_t* argv[],
	std::wstring& LocalFile,
	std::wstring& AzureStorageAccountName,
	std::wstring& AzureStorageAccountKey,
	std::wstring& AzureStorageContainer,
	std::wstring& AzureStorageBlobName,
	unsigned long& chunksize,
	int& countThreads,
	unsigned long& updateblocklistperiod,
	bool& verbose
)
{
	chunksize = 1024 * 1024 * 4; // KB to MB * 4
	countThreads = 4;
	updateblocklistperiod = 0;
	verbose = false;
	bool result = false;

	try
	{
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
				else if (option == L"--blob")
				{
					if (--argc != 0)
						AzureStorageBlobName = *++argv;
				}
				else if (option == L"--file")
				{
					if (--argc != 0)
						LocalFile = *++argv;
				}
				else if (option == L"--threadcount")
				{
					if (--argc != 0)
					{
						countThreads = _wtoi(*++argv);
						if (countThreads < 1 || countThreads > 64)
							return result;
					}
				}
				else if (option == L"--chunksize")
				{
					if (--argc != 0)
					{
						chunksize = (unsigned long)_wtol(*++argv);
						if (chunksize < 1024 || chunksize >(1024 * 1024 * 4))
							return result;
					}
				}
				else if (option == L"--updateblocklistperiod")
				{
					if (--argc != 0)
					{
						updateblocklistperiod = (unsigned long)_wtol(*++argv);
					}
				}
				else if (option == L"--verbose")
				{
					verbose = true;
				}
				else
					return result;
			}
		}
		if ((AzureStorageAccountName.length() > 0) &&
			(AzureStorageAccountKey.length() > 0) &&
			(AzureStorageContainer.length() > 0) &&
			(LocalFile.length() > 0))
		{
			if (AzureStorageBlobName.length() == 0)
			{
				std::wstring folder;
				splitpath(LocalFile, folder, AzureStorageBlobName);
				if (AzureStorageBlobName.length() > 0)
					result = true;
			}
			else
				result = true;
		}
	}
	catch (...)
	{
		result = false;
	}
	return result;
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


int _tmain(int argc, wchar_t* argv[])
{
	
	std::wstring LocalFile = U("");
	std::wstring AzureStorageAccountName = U("");
	std::wstring AzureStorageAccountKey = U("");
	std::wstring AzureStorageContainer = U("");
	std::wstring AzureStorageBlobName = U("");
	unsigned long chunksize = 0;
	int countThreads = 0;
	unsigned long  updateblocklistperiod = 0;
	bool verbose = false;
	azure::storage::cloud_storage_account StorageAccount;
	azure::storage::cloud_blob_client StorageClient;
	_setmode(_fileno(stdout), _O_U16TEXT);

	bool result = ParseCommandLine(argc, argv, LocalFile, AzureStorageAccountName, AzureStorageAccountKey, AzureStorageContainer, AzureStorageBlobName, chunksize, countThreads, updateblocklistperiod, verbose);
	if (result == false)
	{
		ucout << "Azure Storage Upload Command line tool: syntax error" << std::endl;
		ucout << "Syntax:" << std::endl;
		ucout << "UploadCpp.exe --file \"<Path to your local file>\" " << std::endl;
		ucout << "             --account \"<Your Azure Storage Account Name>\" --key \"<Your Azure Storage Account Key>\" " << std::endl;
		ucout << "             --container \"<Your Azure Storage Container>\" [--blob \"<Your Azure Storage BlobName>\"]" << std::endl;
		ucout << "            [--chunksize <ChunkSize Max 4MB default 4MB>] " << std::endl;
		ucout << "            [--threadcount <Number of threads Max 64 default 4>] " << std::endl;
		ucout << "            [--updateblocklistperiod <block list update period in ms>] " << std::endl;
		ucout << "            [--verbose] " << std::endl;
		return 0;
	}

	if(GetAzureStorageClient(AzureStorageAccountName.c_str(), AzureStorageAccountKey.c_str(), StorageClient)==true)
	{
		try
		{
			azure::storage::cloud_blob_container Container;
			azure::storage::cloud_blob_directory Directory;
			ucout << "Connected to Storage Account: " << AzureStorageAccountName.c_str() << std::endl;
			if (AzureStorageContainer.length() > 0)
			{
				Container = StorageClient.get_container_reference(AzureStorageContainer);
				if (Container.exists())
				{
					// Search in the container 
					ucout << "Uploading file: " << LocalFile << " ..." << std::endl;
					BlockUpload *blkup = new BlockUpload(countThreads, chunksize, updateblocklistperiod,verbose);
					blkup->ConnectToAzureStorage(AzureStorageAccountName, AzureStorageAccountKey, AzureStorageContainer);

					bool rc = blkup->UploadFile(LocalFile, AzureStorageBlobName);
					if (!rc)
					{
						ucout << "Unable to open file";
					}
					else
					{
						// show perf timers
						unsigned long mb = blkup->total_bytes / (1024 * 1024);
						float MBs = ((float)mb / blkup->elapsed_secs);
						float mbps = MBs * 8;
						ucout << "Threads: " << countThreads << ". ChunkSize: " << chunksize << ". Bytes: " << blkup->total_bytes << std::endl;
						ucout << "Time: " << blkup->elapsed_secs << " seconds, " << mbps << " Mbps" << ", " << MBs << " MB/s" << std::endl;
					}

					delete blkup;
				}
				else
				{
					ucout << "Azure Storage Container not exists: " << AzureStorageContainer.c_str() << std::endl;
					return 0;
				}

			}
		}
		catch (...)
		{
			ucout << "Azure Storage Upload - Exception" << std::endl;
			return 0;
		}
	}
	return 1;
}

