#include <was/storage_account.h>
#include <was/blob.h>

azure::storage::cloud_storage_account cloud_storage_account_parse(const unsigned short *storage_connection_string)
{
	auto str = utility::string_t((wchar_t *)storage_connection_string);
	return azure::storage::cloud_storage_account::parse(str);
}

azure::storage::cloud_blob_container cloud_blob_client_get_container_reference(azure::storage::cloud_blob_client blob_client, const unsigned short *container_name)
{
	return blob_client.get_container_reference((const wchar_t *)container_name);
}

azure::storage::cloud_block_blob cloud_blob_container_get_block_blob_reference(azure::storage::cloud_blob_container container, const unsigned short *blob_name)
{
	auto str = utility::string_t((wchar_t *)blob_name);
	return container.get_block_blob_reference(str);
}

void cloud_block_blob_upload_text(azure::storage::cloud_block_blob blob, const unsigned short *content)
{
	auto str = utility::string_t((wchar_t *)content);
	blob.upload_text(str);
}

void TestStringOut(PVOID pStrOut)
{
	auto str = utility::string_t((wchar_t *)L"FooBarZedBletch");
	utility::string_t *pStrTmp = (utility::string_t *)pStrOut;
	*pStrTmp = str;
}