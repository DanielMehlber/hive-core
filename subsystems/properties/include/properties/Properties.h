#ifndef PROPERTIES_H
#define PROPERTIES_H

#ifdef _WIN32
// For Windows (MSVC compiler)
#ifdef EXPORT_DLL

// When building the library
#define PROPERTIES_API __declspec(dllexport)
#else

// When using the library
#define PROPERTIES_API __declspec(dllimport)
#endif
#else
// For other platforms (Linux, macOS)
#define PROPERTIES_API
#endif

#endif // PROPERTIES_H
