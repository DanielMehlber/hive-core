#ifndef SERVICES_H
#define SERVICES_H

#ifdef _WIN32
// For Windows (MSVC compiler)
#ifdef EXPORT_DLL

// When building the library
#define SERVICES_API __declspec(dllexport)
#else

// When using the library
#define SERVICES_API __declspec(dllimport)
#endif
#else
// For other platforms (Linux, macOS)
#define SERVICES_API
#endif

#endif // SERVICES_H
