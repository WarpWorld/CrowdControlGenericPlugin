#pragma once
#include "DLLConfig.hpp"
#include "CCEffectInstance.hpp"
#include "CCEffectInstanceTimed.hpp"
#include <string>

class CC_API RPC {
public:
	static void Success(CCEffectInstance& instance);
	static void Success(std::string id, int timeRemaining, long startTime);

	static void FailTemporarily(CCEffectInstance& instance);
	static void FailTemporarily(std::string id, int timeRemaining, long startTime, const std::string& message = "");

	static void FailPermanently(CCEffectInstance& instance);
	static void FailPermanently(std::string id, const std::string& message = "");
	static void TimedBegin(CCEffectInstanceTimed& instance);
	static void TimedPause(CCEffectInstanceTimed& instance);
	static void TimedResume(CCEffectInstanceTimed& instance);
	static void TimedEnd(CCEffectInstanceTimed& instance);
	static void MenuAvailable(CCEffectBase& instance);
	static void MenuUnavailable(CCEffectBase& instance);
	static void MenuVisible(CCEffectBase& instance);
	static void MenuHidden(CCEffectBase& instance);

	// Sends an effectReport RPC ("menuVisible", "menuHidden", "menuAvailable", "menuUnavailable")
	// for a registered effect ID. Returns false if the effect is unknown.
	static bool ReportEffectStatus(const std::string& effectID, const std::string& status);

	// Sends a packMetadataChanged RPC with the given metadata JSON object.
	static void PackMetadataChanged(const std::string& metadataJson);
private:
	static void Send(CCEffectBase& instance, const std::string& command);
	static void Send(CCEffectInstance& instance, const std::string& command);
	static void Send(std::string id, const std::string& command);
	static void Send(const std::string& command, std::string id, int timeRemaining, long startTime, const std::string& message = "");
};

