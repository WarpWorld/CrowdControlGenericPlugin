#pragma once  

#define DLL_EXPORT __declspec(dllexport)    //shortens __declspec(dllexport) to DLL_EXPORT

#ifdef __cplusplus        //if C++ is used convert it to C to prevent C++'s name mangling of method names
extern "C"
{
#endif

	bool DLL_EXPORT getInvertedBool(bool boolState);
	int DLL_EXPORT getIntPlusPlus(int lastInt);
	float DLL_EXPORT getCircleArea(float radius);
	void DLL_EXPORT AddNewBasicEffect(char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray);
	void DLL_EXPORT AddNewTimedEffect(char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray, float duration);
	void DLL_EXPORT AddNewParameterEffect(char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray);
	void DLL_EXPORT AddParameterOption(char* name, char* paramName, char** options);
	void DLL_EXPORT AddParamaterMinMax(char* name, char* paramName, int min, int max);

	void DLL_EXPORT EffectSuccess(char * id);
	void DLL_EXPORT EffectFailure(char * id);

	float DLL_EXPORT *getVector4(float x, float y, float z, float w);

#ifdef __cplusplus
}
#endif
