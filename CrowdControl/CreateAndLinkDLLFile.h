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
	float DLL_EXPORT *getVector4(float x, float y, float z, float w);

#ifdef __cplusplus
}
#endif
