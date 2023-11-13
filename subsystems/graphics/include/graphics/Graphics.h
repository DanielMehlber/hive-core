#ifndef GRAPHICS_H
#define GRAPHICS_H

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

#endif // GRAPHICS_H
