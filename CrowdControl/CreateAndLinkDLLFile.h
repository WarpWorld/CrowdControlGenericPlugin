#pragma once  

#include "DLLConfig.hpp"  // Include this to get CC_API definition

// Basic utility functions
bool CC_API getInvertedBool(bool boolState);
int CC_API getIntPlusPlus(int lastInt);
float CC_API getCircleArea(float radius);
float CC_API *getVector4(float x, float y, float z, float w);

// Crowd Control effect functions - keep C++ names for mangling
void CC_API AddNewBasicEffect(char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray);
void CC_API AddNewTimedEffect(char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray, float duration);
void CC_API AddNewParameterEffect(char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray);
void CC_API AddParameterOption(char* name, char* paramName, char** options);
void CC_API AddParameterMinMax(char* name, char* paramName, int min, int max);

void CC_API EffectSuccess(char * id);
void CC_API EffectFailure(char * id);

// JWT Token functions
char* CC_API GetOriginID();
char* CC_API GetProfileType();
char* CC_API GetInteractionURL();
char* CC_API GetStreamerName();
bool CC_API IsJWTTokenValid();
