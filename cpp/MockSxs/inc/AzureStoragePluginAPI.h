#ifdef AZURESTORAGEPLUGIN_EXPORTS
#define AZURESTORAGEPLUGIN_API __declspec(dllexport)
#else
#define AZURESTORAGEPLUGIN_API __declspec(dllimport)
#endif
AZURESTORAGEPLUGIN_API DIO::IStoragePluginPtr CreateAzureStoragePlugin(void);
