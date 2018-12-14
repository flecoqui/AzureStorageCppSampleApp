// Upload.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

const unsigned int ONEMB = 1 * 1024 * 1024;
const unsigned int FOURMB = 4 * ONEMB;
const unsigned int HALFMB = ONEMB / 2;
const unsigned int EIGHTHMB = ONEMB / 8;
const unsigned int SIXTEENTHMB = ONEMB / 16;

// Define the connection-string with your values.
const utility::string_t storage_connection_string(U("DefaultEndpointsProtocol=https;AccountName=<YourName>;AccountKey=<YourKey>"));

void CreateBlockId(PTSTR pszBlockId, DWORD dwSize)
{
	UUID uidBlockId;
	auto ret = UuidCreate(&uidBlockId);
	CryptBinaryToString((PBYTE)&uidBlockId, sizeof(uidBlockId), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, pszBlockId, &dwSize);
}

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

		// Open local file
		//concurrency::streams::istream input_stream = concurrency::streams::file_stream<uint8_t>::open_istream(U("..\\bbb_sunflower_1080p_60fps_normal.mp4")).get();
		//concurrency::streams::istream input_stream = concurrency::streams::file_stream<uint8_t>::open_istream(U("..\\foo.bin")).get();

		std::ifstream file(L"..\\foo.bin", std::ios::in | std::ios::binary | std::ios::ate);
		size_t fileSize = file.tellg();

		#define FOURMB (4*1024*1024)
		#define	BLOCKSIZE HALFMB
		std::vector<azure::storage::block_list_item> blockList;
		int numBlocks = fileSize / BLOCKSIZE + 1;
		int numBlocksWritten = 0;
		std::vector<uint8_t> firstBuffer;
		utility::string_t firstBlockId;

		do {
			TCHAR szBlockId[32];
			DWORD dwSize = sizeof(szBlockId) / sizeof(TCHAR);

			CreateBlockId(szBlockId, dwSize);
			utility::string_t blockId(szBlockId);

			size_t seekPos = BLOCKSIZE*blockList.size();
			file.seekg(seekPos);

			size_t actualBlockSize = BLOCKSIZE;
			if ((fileSize - numBlocksWritten*BLOCKSIZE) < BLOCKSIZE) {
				actualBlockSize = (fileSize - numBlocksWritten*BLOCKSIZE);
			}

			std::vector<uint8_t> readbuf(actualBlockSize);
			file.read((char*)&readbuf[0], actualBlockSize);

			if (numBlocksWritten == 0) {
				firstBuffer = std::vector<uint8_t>(readbuf);
				firstBlockId = utility::string_t(szBlockId);
			}

			PBYTE pBuffer = &readbuf[0];
			concurrency::streams::rawptr_buffer<uint8_t> buffer(pBuffer, actualBlockSize, std::ios::in);
			
			auto stream = buffer.create_istream();
			blockBlob.upload_block(blockId, stream, L"");

			stream.close();
			buffer.close();

			azure::storage::block_list_item bli(blockId);
			blockList.push_back(bli);
			blockBlob.upload_block_list(blockList);
			numBlocksWritten++;

			std::wcout << U("id = ") << bli.id() << U(", size = ") << actualBlockSize << std::endl;

			if (numBlocks == numBlocksWritten) {
				// Update first block and upload it
				firstBuffer[0] = 'Q';
				firstBuffer[1] = 'S';
				auto stream = concurrency::streams::bytestream::open_istream(firstBuffer);
				blockBlob.upload_block(firstBlockId, stream, L"");
				blockBlob.upload_block_list(blockList);
			}
		} while (numBlocksWritten < numBlocks);
	} catch (const std::exception& e) {
		std::wcout << U("Error: ") << e.what() << std::endl;
	}
	return 0;
}

