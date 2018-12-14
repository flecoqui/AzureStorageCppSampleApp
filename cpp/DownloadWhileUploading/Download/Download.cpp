// Download.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

// Define the connection-string with your values.
const utility::string_t storage_connection_string(U("DefaultEndpointsProtocol=https;AccountName=<YourName>;AccountKey=<YourKey>"));

int main()
{
	try {
		// Retrieve storage account from connection string.
		azure::storage::cloud_storage_account storage_account = azure::storage::cloud_storage_account::parse(storage_connection_string);

		// Create the blob client.
		azure::storage::cloud_blob_client blob_client = storage_account.create_cloud_blob_client();

		// Retrieve a reference to a container.
		azure::storage::cloud_blob_container container = blob_client.get_container_reference(U("media"));

		// Create the container if it doesn't already exist.
		container.create_if_not_exists();

		// Retrieve reference to a blob named "bbb.mp4".
		azure::storage::cloud_block_blob blockBlob = container.get_block_blob_reference(U("bbb2.mp4"));

		azure::storage::block_list_item *pLatestDownloadedBlock = NULL;
		bool done = false;
		size_t bytesRead = 0;
		size_t blocksRead = 0;
		std::ofstream file(L"..\\foo-dl.bin", std::ios::out | std::ios::binary | std::ios::ate);

		do {
			auto blockList = blockBlob.download_block_list();
			if (blocksRead == blockList.size()) {
				::Sleep(1000);
				continue;
			}

			size_t tmpBlockSizes = 0;
			for each (auto bli in blockList) {
				if (tmpBlockSizes < bytesRead) {
					tmpBlockSizes += bli.size();
					continue;
				}

				std::wcout << U("id = ") << bli.id() << U(", mode = ") << bli.mode() << U(", size = ") << bli.size() << std::endl;

				concurrency::streams::container_buffer<std::vector<uint8_t>> download_buffer;
				blockBlob.download_range_to_stream(download_buffer.create_ostream(), bytesRead, bli.size());

				auto coll = download_buffer.collection();
				file.write((const char *)&coll[0], bli.size());
				bytesRead += bli.size();
				tmpBlockSizes += bli.size();
				blocksRead++;
			}
			file.flush();

		} while (!done);
		file.close();
	} catch (const std::exception& e) {
		std::wcout << U("Error: ") << e.what() << std::endl;
	}
	return 0;
}

