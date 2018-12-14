
#ifdef UTILS_EXPORTS
#define UTILS_API __declspec(dllexport)
#else
#define UTILS_API __declspec(dllimport)
#endif

extern "C" {
	UTILS_API int Multiply(int x, int y);
}