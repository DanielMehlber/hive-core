#ifndef SERVICES_H
#define SERVICES_H

#ifdef _WIN32
// For Windows (MSVC compiler)
#ifdef EXPORT_DLL

// When building the library
#define GRAPHICS_API __declspec(dllexport)
#else

// When using the library
#define GRAPHICS_API __declspec(dllimport)
#endif
#else
// For other platforms (Linux, macOS)
#define GRAPHICS_API
#endif

#endif // SERVICES_H
