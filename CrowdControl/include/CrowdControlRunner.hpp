#pragma once 

#include <string> 
#include <functional>
#include <vector>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include "CCEffectBase.hpp"
#include "CCEffectTimed.hpp"
#include "StreamUser.hpp"
#include "DLLConfig.hpp"
#include <atomic>

class CC_API CrowdControlRunner {
public:
	int Run();
	void EngineSet();
	int CommandID();
	void ResetCommandCode();
	char* TestCharArray();
	static void AddBasicEffect(char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray);
	static void AddTimedEffect(char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray, float duration);
	static void AddParameterEffect(char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray);
	static void AddParameterOption(char* name, char* paramName, char** options);
	static void AddParameterMinMax(char* name, char* paramName, int min, int max);

	static void Success(char * name);
	static void Fail(char * name);
	static char* EngineEffect();

	static std::atomic<int> commandCode;

	static std::string connectionID;
	static std::string token;
	static std::string gamePackID;
	static std::string gameName;
	static std::string gameSessionID;
	static std::string engine;
	static std::string extMessage;
	static bool sendingPost; 

	static std::atomic<bool> connected;
	static void WriteToSocket(nlohmann::json);
	static void PushToQueue(const std::function<void(const std::wstring&)>& callback, const std::wstring& message);

	static std::unordered_map<std::string, std::shared_ptr<CCEffectBase>> effects;
	static std::unordered_map<std::string, std::shared_ptr<StreamUser>> streamUsers;
	static std::unordered_map<std::string, std::shared_ptr<CCEffectInstanceTimed>> runningEffects;

	static void ChooseSite();
	static std::function<void()> siteCallback;

	CrowdControlRunner();
	~CrowdControlRunner();

	static void StartGameSession();
	static void StopGameSession();
	static void StopAllEffects();

	static const int FPS = 60;

	static void LoginTwitch();
	static void LoginYoutube();
	static void LoginDiscord();

	static void SaveToken();
	static void ClearToken();

	static bool StopEffect(std::string effectID);
	static bool IsRunning(std::string name);
	static bool ResetEffect(std::string effectID);
	static bool PauseEffect(std::string effectID);
	static bool ResumeEffect(std::string effectID);
	
	static void Connect();
	static void Disconnect();
	static std::string JSONManifest();

	static bool HasRunningEffects();
	static bool IsPaused(std::string name);

	static void TestEffect(std::string id, std::map<std::string, std::string> paramPairs = std::map<std::string, std::string>());
	static void TestEffectRemotely(std::string id, std::map<std::string, std::string> paramPairs = std::map<std::string, std::string>());

	static std::string GetMessage() {
		std::string temp = CrowdControlRunner::extMessage;
		CrowdControlRunner::extMessage = "";
		return temp;
	}
};
