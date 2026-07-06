#include "string.h"
#include "CreateAndLinkDLLFile.h"
#include "include/CrowdControlRunner.hpp"
#include "pch.h"

//Exported method that invertes a given boolean.
bool getInvertedBool(bool boolState)
{
	return bool(!boolState);
}

//Exported method that iterates a given int value.
int getIntPlusPlus(int lastInt)
{
	return int(++lastInt);
}

//Exported method that calculates the are of a circle by a given radius.
float getCircleArea(float radius)
{
	return float(3.1416f * (radius * radius));
}

void AddNewBasicEffect(char* id, char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray)
{
	CrowdControlRunner::AddBasicEffect(id, name, desc, price, retries, retryDelay, pendingDelay, sellable, visible, nonPoolable, morality, orderliness, categoriesArray);
}

void AddNewTimedEffect(char* id, char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray, float duration)
{
	CrowdControlRunner::AddTimedEffect(id, name, desc, price, retries, retryDelay, pendingDelay, sellable, visible, nonPoolable, morality, orderliness, categoriesArray, duration);
}

void AddNewParameterEffect(char* id, char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray)
{
	CrowdControlRunner::AddParameterEffect(id, name, desc, price, retries, retryDelay, pendingDelay, sellable, visible, nonPoolable, morality, orderliness, categoriesArray);
}

void AddParameterOption(char* id, char* paramName, char** options) {
	CrowdControlRunner::AddParameterOption(id, paramName, options);
}

void AddParameterMinMax(char* id, char* paramName, int min, int max) {
	CrowdControlRunner::AddParameterMinMax(id, paramName, min, max);
}

void EffectSuccess(char * id) {
	CrowdControlRunner::Success(id);
}

void EffectFailure(char * id) {
	CrowdControlRunner::Fail(id);
}

void EffectFailTemporary(const char* id, const char* message) {
	CrowdControlRunner::FailTemporary(id, message);
}

void EffectFailPermanent(const char* id, const char* message) {
	CrowdControlRunner::FailPermanent(id, message);
}

// Connection lifecycle.
// Heap-allocated and intentionally never freed: the runner's destructor performs
// network calls (StopAllEffects/StopGameSession), which must not run during
// static destruction at DLL unload.
static CrowdControlRunner* runnerInstance = new CrowdControlRunner();

int RunCrowdControl() {
	return runnerInstance->Run();
}

void ConnectCrowdControl() {
	CrowdControlRunner::Connect();
}

void DisconnectCrowdControl() {
	CrowdControlRunner::Disconnect();
}

int GetCommandID() {
	return runnerInstance->CommandID();
}

void ResetCommand() {
	runnerInstance->ResetCommandCode();
}

void SetEngine() {
	runnerInstance->EngineSet();
}

bool SetGameNameAndPackId(char* name, char* packID) {
	if (name == nullptr || packID == nullptr) {
		return false;
	}

	CrowdControlRunner::SetGameNameAndPackID(name, packID);
	return true;
}

char* GetQueuedMessage() {
	return runnerInstance->TestCharArray();
}

char* GetEngineEffect() {
	return CrowdControlRunner::EngineEffect();
}

// Legacy platform login
void LoginTwitch() {
	CrowdControlRunner::LoginTwitch();
}

void LoginYoutube() {
	CrowdControlRunner::LoginYoutube();
}

void LoginDiscord() {
	CrowdControlRunner::LoginDiscord();
}

// Application (appID) auth-code flow
void SetAppID(const char* appID) {
	CrowdControlRunner::SetAppID(appID);
}

void SetPublicClientKey(const char* publicClientKey) {
	CrowdControlRunner::SetAppSecret(publicClientKey);
}

void RequestAuthCode() {
	CrowdControlRunner::RequestAuthCode();
}

char* GetAuthCode() {
	return CrowdControlRunner::GetAuthCodeForUnreal();
}

// Game session control
void SetAutoStartSession(bool autoStart) {
	CrowdControlRunner::autoStartSession = autoStart;
}

void StartSession() {
	CrowdControlRunner::StartGameSession();
}

void StopSession() {
	CrowdControlRunner::StopGameSession();
}

// Running (timed) effect control. The runner's public API is keyed by display name,
// so map the effect ID back to its display name first.
static std::string EffectDisplayNameFromID(const char* effectID) {
	if (effectID == nullptr) {
		return std::string();
	}

	auto it = CrowdControlRunner::effects.find(std::string(effectID));
	if (it == CrowdControlRunner::effects.end()) {
		return std::string();
	}

	return it->second->displayName;
}

bool StopEffectById(const char* effectID) {
	std::string displayName = EffectDisplayNameFromID(effectID);
	return !displayName.empty() && CrowdControlRunner::StopEffect(displayName);
}

bool ResetEffectById(const char* effectID) {
	std::string displayName = EffectDisplayNameFromID(effectID);
	return !displayName.empty() && CrowdControlRunner::ResetEffect(displayName);
}

bool PauseEffectById(const char* effectID) {
	std::string displayName = EffectDisplayNameFromID(effectID);
	return !displayName.empty() && CrowdControlRunner::PauseEffect(displayName);
}

bool ResumeEffectById(const char* effectID) {
	std::string displayName = EffectDisplayNameFromID(effectID);
	return !displayName.empty() && CrowdControlRunner::ResumeEffect(displayName);
}

bool IsEffectRunning(const char* effectID) {
	std::string displayName = EffectDisplayNameFromID(effectID);
	return !displayName.empty() && CrowdControlRunner::IsRunning(displayName);
}

// Effect menu reports & pack metadata
bool ReportEffectStatus(const char* effectID, int status) {
	return CrowdControlRunner::ReportEffect(effectID, status);
}

void SendPackMetadata(const char* metadataJson) {
	CrowdControlRunner::SendPackMetadata(metadataJson);
}

// JWT Token functions for Unreal access
char* GetOriginID() {
	return CrowdControlRunner::GetOriginIDForUnreal();
}

char* GetProfileType() {
	return CrowdControlRunner::GetProfileTypeForUnreal();
}

char* GetInteractionURL() {
	return CrowdControlRunner::GetInteractionURLForUnreal();
}

char* GetStreamerName() {
	return CrowdControlRunner::GetStreamerNameForUnreal();
}

bool IsJWTTokenValid() {
	return CrowdControlRunner::IsJWTTokenValid();
}

//Exported method that adds a vector4 to a given vector4 and returns the sum.
float *getVector4(float x, float y, float z, float w)
{
	float* modifiedVector4 = new float[4];

	modifiedVector4[0] = x + 1.0F;
	modifiedVector4[1] = y + 2.0F;
	modifiedVector4[2] = z + 3.0F;
	modifiedVector4[3] = w + 4.0F;

	return (float*)modifiedVector4;
}

// Custom Effects API exports
void UploadCustomEffects(const char* effectsJson) {
	CrowdControlRunner::UploadCustomEffects(effectsJson);
}

void ClearCustomEffects() {
	CrowdControlRunner::ClearCustomEffects();
}

void DeleteCustomEffects(const char* effectIDsJson) {
	CrowdControlRunner::DeleteCustomEffects(effectIDsJson);
}

char* GetCustomEffects() {
	return CrowdControlRunner::GetCustomEffects();
}
