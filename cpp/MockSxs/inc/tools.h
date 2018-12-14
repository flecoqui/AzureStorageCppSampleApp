
#ifdef TOOLS_EXPORTS
#define TOOLS_API __declspec(dllexport)
#else
#define TOOLS_API __declspec(dllimport)
#endif

extern "C" {
	TOOLS_API int DoIt(int x);
}
