#ifndef JOBSYSTEM_H
#define JOBSYSTEM_H

#ifdef _WIN32
// For Windows (MSVC compiler)
#ifdef EXPORT_DLL

// When building the library
#define JOBSYSTEM_API __declspec(dllexport)
#else

// When using the library
#define JOBSYSTEM_API __declspec(dllimport)
#endif
#else
#define JOBSYSTEM_API
#endif

#endif /* JOBSYSTEM_H */
