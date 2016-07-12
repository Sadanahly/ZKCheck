#ifndef configH
#define configH

#ifdef _WIN32
#  ifdef ZKCHECKLIB_EXPORT
#    define ZKCHECKLIB __declspec(dllexport)
#  elif defined(ZKCHECKLIB_IMPORT)
#    define ZKCHECKLIB __declspec(dllimport)
#  else
#    define ZKCHECKLIB
#  endif
#else
#  define ZKCHECKLIB
#endif

// MS Visual C++ ÄÚ´æÐ¹Â©µ÷ÊÔ¸ú×Ù
#if defined(_MSC_VER) && defined(_DEBUG)
#  define _CRTDBG_MAP_ALLOC
#  include <crtdbg.h>
#endif

#include <string>
static const std::string emptyString;

#endif
