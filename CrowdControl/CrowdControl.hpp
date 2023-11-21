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

class CC_API CrowdControl {
public:
	int Run();

	static std::string connectionID;
	static std::string token;
	static std::string gamePackID;
	static std::string gameName;
	static std::string gameSessionID;
	static bool sendingPost;
	static bool connected;
	static std::unordered_map<std::string, std::shared_ptr<CCEffectBase>> effects;
	static std::unordered_map<std::string, std::shared_ptr<StreamUser>> streamUsers;
	static std::unordered_map<std::string, std::shared_ptr<CCEffectInstanceTimed>> runningEffects;

	CrowdControl();
	~CrowdControl();

	static void PushToQueue(const std::function<void(const std::wstring&)>& callback, const std::wstring& message);
	static void StartGameSession();
	static void StopGameSession();
	static void WriteToSocket(nlohmann::json);
	static const int FPS = 60;

	static bool PauseEffect(std::string effectID);
	static bool ResumeEffect(std::string effectID);
	static bool ResetEffect(std::string effectID);
	static bool StopEffect(std::string effectID);
	static void StopAllEffects();

	static void SaveToken();
	static void ClearToken();
	static void Connect();
	static void Disconnect();

	static void TestEffect(std::string id, std::map<std::string, std::string> paramPairs = std::map<std::string, std::string>());
	static void TestEffectRemotely(std::string id, std::map<std::string, std::string> paramPairs = std::map<std::string, std::string>());
	static bool HasRunningEffects();
	static bool IsRunning(std::string name);
	static bool IsPaused(std::string name);

	static void LoginTwitch();
	static void LoginYoutube();
	static void LoginDiscord();
	static std::string JSONManifest();
};
