#pragma once

azure::storage::cloud_storage_account cloud_storage_account_parse(const unsigned short *storage_connection_string);
azure::storage::cloud_blob_container cloud_blob_client_get_container_reference(azure::storage::cloud_blob_client blob_client, const unsigned short *container_name);
azure::storage::cloud_block_blob cloud_blob_container_get_block_blob_reference(azure::storage::cloud_blob_container container, const unsigned short *blob_name);
void cloud_block_blob_upload_text(azure::storage::cloud_block_blob blob, const unsigned short *content);

void TestStringOut(PVOID pStrOut);