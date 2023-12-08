#ifndef MESSAGING_H
#define MESSAGING_H

#ifdef _WIN32
// For Windows (MSVC compiler)
#ifdef EXPORT_DLL

// When building the library
#define EVENT_API __declspec(dllexport)
#else

// When using the library
#define EVENT_API __declspec(dllimport)
#endif
#else
// For other platforms (Linux, macOS)
#define EVENT_API
#endif

#endif /* MESSAGING_H */
