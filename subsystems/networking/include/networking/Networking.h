#ifndef NETWORKING_H
#define NETWORKING_H

#ifdef _WIN32
// For Windows (MSVC compiler)
#ifdef EXPORT_DLL

// When building the library
#define NETWORKING_API __declspec(dllexport)
#else

// When using the library
#define NETWORKING_API __declspec(dllimport)
#endif
#else
// For other platforms (Linux, macOS)
#define NETWORKING_API
#endif

#endif // NETWORKING_H
