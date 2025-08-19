#pragma once

// Define CC_API for proper DLL export/import
#ifdef CC_EXPORTS
    // When building the DLL, export the functions
    #define CC_API __declspec(dllexport)
#else
    // When using the DLL, import the functions
    #define CC_API __declspec(dllimport)
#endif
