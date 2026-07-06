#pragma once

#define DLL_EXPORT __declspec(dllexport)    //shortens __declspec(dllexport) to DLL_EXPORT

#ifdef __cplusplus        //if C++ is used convert it to C to prevent C++'s name mangling of method names
extern "C"
{
#endif

	bool DLL_EXPORT getInvertedBool(bool boolState);
	int DLL_EXPORT getIntPlusPlus(int lastInt);
	float DLL_EXPORT getCircleArea(float radius);
	void DLL_EXPORT AddNewBasicEffect(char* id, char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray);
	void DLL_EXPORT AddNewTimedEffect(char* id, char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray, float duration);
	void DLL_EXPORT AddNewParameterEffect(char* id, char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray);
	void DLL_EXPORT AddParameterOption(char* id, char* paramName, char** options);
	void DLL_EXPORT AddParameterMinMax(char* id, char* paramName, int min, int max);

	// Effect responses
	void DLL_EXPORT EffectSuccess(char* id);
	void DLL_EXPORT EffectFailure(char* id);
	void DLL_EXPORT EffectFailTemporary(const char* id, const char* message);
	void DLL_EXPORT EffectFailPermanent(const char* id, const char* message);

	// Connection lifecycle (game-engine entry points)
	int  DLL_EXPORT RunCrowdControl();
	void DLL_EXPORT ConnectCrowdControl();
	void DLL_EXPORT DisconnectCrowdControl();
	int  DLL_EXPORT GetCommandID();
	void DLL_EXPORT ResetCommand();
	void DLL_EXPORT SetEngine();
	bool DLL_EXPORT SetGameNameAndPackId(char* name, char* packID);
	DLL_EXPORT char* GetQueuedMessage();
	DLL_EXPORT char* GetEngineEffect();

	// Legacy platform login (streamer accounts)
	void DLL_EXPORT LoginTwitch();
	void DLL_EXPORT LoginYoutube();
	void DLL_EXPORT LoginDiscord();

	// Application (appID) auth-code flow
	void DLL_EXPORT SetAppID(const char* appID);
	void DLL_EXPORT SetPublicClientKey(const char* publicClientKey);
	void DLL_EXPORT RequestAuthCode();
	DLL_EXPORT char* GetAuthCode(); // one-shot JSON {"code":...,"url":...}, empty until an auth code arrives

	// Game session control
	void DLL_EXPORT SetAutoStartSession(bool autoStart);
	void DLL_EXPORT StartSession();
	void DLL_EXPORT StopSession();

	// Running (timed) effect control — by effect ID
	bool DLL_EXPORT StopEffectById(const char* effectID);
	bool DLL_EXPORT ResetEffectById(const char* effectID);
	bool DLL_EXPORT PauseEffectById(const char* effectID);
	bool DLL_EXPORT ResumeEffectById(const char* effectID);
	bool DLL_EXPORT IsEffectRunning(const char* effectID);

	// Effect menu reports & pack metadata
	bool DLL_EXPORT ReportEffectStatus(const char* effectID, int status); // 0=visible 1=hidden 2=available 3=unavailable
	void DLL_EXPORT SendPackMetadata(const char* metadataJson);

	float DLL_EXPORT* getVector4(float x, float y, float z, float w);

	// JWT token info
	DLL_EXPORT char* GetOriginID();
	DLL_EXPORT char* GetProfileType();
	DLL_EXPORT char* GetInteractionURL();
	DLL_EXPORT char* GetStreamerName();
	DLL_EXPORT bool  IsJWTTokenValid();

	// Custom Effects API
	DLL_EXPORT void UploadCustomEffects(const char* effectsJson);
	DLL_EXPORT void ClearCustomEffects();
	DLL_EXPORT void DeleteCustomEffects(const char* effectIDsJson);
	DLL_EXPORT char* GetCustomEffects();

#ifdef __cplusplus
}
#endif
