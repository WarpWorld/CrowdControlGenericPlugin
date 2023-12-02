#include "RPC.hpp"
#include "CrowdControlRunner.hpp"
#include <random>
#include <chrono>
#include <nlohmann/json.hpp>

void RPC::Success(CCEffectInstance& instance) {
	Send(instance, "success");
}

void RPC::FailTemporarily(CCEffectInstance& instance) {
	Send(instance, "failTemporary");
}

void RPC::FailPermanently(CCEffectInstance& instance) {
	Send(instance, "failPermanent");
}

void RPC::TimedBegin(CCEffectInstanceTimed& instance) {
	Send(instance, "timedBegin");
}

void RPC::TimedPause(CCEffectInstanceTimed& instance) {
	Send(instance, "timedPause");
}

void RPC::TimedResume(CCEffectInstanceTimed& instance) {
	Send(instance, "timedResume");
}

void RPC::TimedEnd(CCEffectInstanceTimed& instance) {
	Send(instance, "timedEnd");
}

void RPC::MenuAvailable(CCEffectBase& instance) {
	Send(instance, "menuAvailable");
}

void RPC::MenuUnavailable(CCEffectBase& instance) {
	Send(instance, "menuUnavailable");
}

void RPC::MenuVisible(CCEffectBase& instance) {
	Send(instance, "menuVisible");
}

void RPC::MenuHidden(CCEffectBase& instance) {
	Send(instance, "menuHidden");
}

std::string RandomString() {
	const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	std::random_device rd;
	std::mt19937 generator(rd());
	std::uniform_int_distribution<> distribution(0, chars.size() - 1);

	std::string random_string;
	for (size_t i = 0; i < 26; ++i) {
		random_string += chars[distribution(generator)];
	}

	return random_string;
}

int EpochSeconds() {
	auto now = std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);

	return seconds.count();
}

void RPC::Send(CCEffectInstance& instance, const std::string& command) {
	if (instance.id.find("-") == std::string::npos) { // We don't RPC local test effects
		return;
	}

	nlohmann::json data;
	data["token"] = CrowdControlRunner::token;
	data["call"]["method"] = "effectResponse";
	data["call"]["args"] = nlohmann::json::array();

	nlohmann::json args;

	CCEffectInstanceTimed* timedInstance = dynamic_cast<CCEffectInstanceTimed*>(&instance);
	if (timedInstance != nullptr) {
		args["timeRemaining"] = timedInstance->TimeRemaining();
	}
	else {
		args["timeRemaining"] = 0;
	}

	args["request"] = instance.id;

	args["id"] = RandomString();
	args["stamp"] = instance.unscaledStartTime;
	args["status"] = command;
	args["stamp"] = EpochSeconds();
	args["message"] = "";

	data["call"]["args"].push_back(args);

	data["call"]["id"] = "";
	data["call"]["type"] = "call";

	nlohmann::json jsonObj;
	jsonObj["action"] = "rpc";
	jsonObj["data"] = data.dump();
	CrowdControlRunner::WriteToSocket(jsonObj);
}

void RPC::Send(CCEffectBase& effect, const std::string& command) {
	nlohmann::json data;
	data["token"] = CrowdControlRunner::token;
	data["call"]["method"] = "effectReport";
	data["call"]["args"] = nlohmann::json::array();

	nlohmann::json args;

	args["id"] = RandomString();
	args["ids"] = nlohmann::json::array();
	args["ids"].push_back(effect.id);
	args["status"] = command;
	args["stamp"] = EpochSeconds();
	args["effectType"] = "game";
	args["identifierType"] = "effect";

	data["call"]["args"].push_back(args);

	data["call"]["id"] = "string";
	data["call"]["type"] = "call";

	nlohmann::json jsonObj;
	jsonObj["action"] = "rpc";
	jsonObj["data"] = data.dump();
	CrowdControlRunner::WriteToSocket(jsonObj);
}
