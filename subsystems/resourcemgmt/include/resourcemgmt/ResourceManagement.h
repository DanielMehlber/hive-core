#ifndef RESOURCEMANAGEMENT_H
#define RESOURCEMANAGEMENT_H

#ifdef _WIN32
// For Windows (MSVC compiler)
#ifdef EXPORT_DLL

// When building the library
#define RESOURCE_API __declspec(dllexport)
#else

// When using the library
#define RESOURCE_API __declspec(dllimport)
#endif
#else
// For other platforms (Linux, macOS)
#define RESOURCE_API
#endif

#endif // RESOURCEMANAGEMENT_H
