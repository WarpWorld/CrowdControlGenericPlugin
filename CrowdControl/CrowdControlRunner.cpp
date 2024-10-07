#include "include/CrowdControlRunner.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <cctype>
#include <windows.h>
#include <fstream>
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <functional>
#include <thread>
#include <cpprest/json.h>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <conio.h>
#include <random>
#include <atomic>

#include "include/CCEffectBase.hpp"
#include "include/CCEffect.hpp"
#include "include/CCEffectTimed.hpp"
#include "include/CCEffectParameters.hpp"
#include "include/CCEffectInstanceParameters.hpp"
#include "include/CCEffectTest.hpp"
#include "include/CCEffectTimedTest.hpp"
#include "include/CCEffectParametersTest.hpp"
#include "include/EffectResult.hpp"
#include "include/RPC.hpp"
#include "include/StreamBuf.hpp"
#include "include/ServerRequests.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

websocket::stream<beast::ssl_stream<tcp::socket>> *ccSocket = nullptr;

std::string CrowdControlRunner::connectionID;
std::string CrowdControlRunner::token;
std::string CrowdControlRunner::gamePackID;
std::string CrowdControlRunner::gameName;
std::string CrowdControlRunner::gameSessionID;
std::atomic<bool> CrowdControlRunner::connected;
bool CrowdControlRunner::sendingPost = false;
static std::queue<std::pair<std::function<void(const std::wstring &)>, std::wstring>> PostGetResponses;

static std::queue<std::shared_ptr<CCEffectInstance>> engineQueuedEffects;

std::unordered_map<std::string, std::shared_ptr<CCEffectBase>> CrowdControlRunner::effects;
std::unordered_map<std::string, std::shared_ptr<StreamUser>> streamUsers;
std::unordered_map<std::string, std::shared_ptr<CCEffectInstanceTimed>> CrowdControlRunner::runningEffects;
std::unordered_map<std::string, std::queue<std::shared_ptr<CCEffectInstanceTimed>>> haltedTimers;
std::chrono::steady_clock::time_point program_start = std::chrono::steady_clock::now();
std::chrono::steady_clock::time_point effect_delay = std::chrono::steady_clock::now();
std::chrono::steady_clock::time_point update_time = std::chrono::steady_clock::now();

std::vector<std::string> effectIDs;
std::vector<std::string> effectInstanceIDs;
std::queue<std::shared_ptr<CCEffectInstance>> pendingQueue;
std::shared_ptr<StreamUser> streamer;
int maxRetries = 3;
bool startSessionAutomatically = true;
std::shared_ptr<StreamUser> localUser;
std::string CrowdControlRunner::engine;
std::string CrowdControlRunner::extMessage;

std::atomic<int> CrowdControlRunner::commandCode;

Streambuf customBuf;

void CrowdControlRunner::WriteToSocket(nlohmann::json jsonObj)
{
	std::cout << "SENT: " + jsonObj.dump() << "\n";
	ccSocket->write(net::buffer(std::string(jsonObj.dump())));
}

CrowdControlRunner::CrowdControlRunner()
{
}

CrowdControlRunner::~CrowdControlRunner()
{
	CrowdControlRunner::StopAllEffects();
	CrowdControlRunner::StopGameSession();
}

void CrowdControlRunner::PushToQueue(const std::function<void(const std::wstring &)> &callback, const std::wstring &message)
{
	PostGetResponses.push({callback, message});
}

long GetMillisecondsSinceOffset(std::chrono::steady_clock::time_point offset)
{
	auto now = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - offset);
	return duration.count();
}

template <typename T>
std::shared_ptr<T> CreateEffectInstance(std::shared_ptr<CCEffectBase> effect, std::shared_ptr<StreamUser> sender, std::string id)
{
	auto instance = std::make_shared<T>(); // Create a shared pointer of type T
	instance->effect = std::dynamic_pointer_cast<CCEffectBase>(effect);
	instance->sender = sender;
	instance->id = id;
	instance->unscaledStartTime = GetMillisecondsSinceOffset(program_start);
	std::cout << "Creating instance of type: " << typeid(T).name() << std::endl;

	return instance;
}

void StopGameSessionProcess(const std::wstring &response)
{
	if (CrowdControlRunner::gameSessionID.empty())
	{
		return;
	}

	CrowdControlRunner::gameSessionID = "";
	std::cout << "Stopped the Crowd Control Session!" << "\n";
}

void StartGameSessionProcess(const std::wstring &response)
{
	if (ServerRequests::makingRequest)
	{
		return;
	}

	nlohmann::json startJSON = nlohmann::json::parse(response);
	CrowdControlRunner::gameSessionID = startJSON["gameSessionID"];
	Streambuf::Important("Started the Crowd Control Session!");
}

void CrowdControlRunner::StopGameSession()
{
	nlohmann::json stopMessage;
	stopMessage["gamePackID"] = CrowdControlRunner::gamePackID;
	web::json::value webJson = web::json::value::parse(stopMessage.dump());
	ServerRequests::SendPost(L"stop", StartGameSessionProcess, webJson, true);
}

void CrowdControlRunner::StartGameSession()
{
	nlohmann::json startMessage;
	startMessage["gamePackID"] = CrowdControlRunner::gamePackID;
	web::json::value webJson = web::json::value::parse(startMessage.dump());
	ServerRequests::SendPost(L"start", StartGameSessionProcess, webJson, true);
}

void SendHello()
{
	nlohmann::json helloObj;
	helloObj["action"] = "whoami";
	CrowdControlRunner::WriteToSocket(helloObj);
}

void ProcessSubResult(std::string message)
{
	nlohmann::json jsonObj = nlohmann::json::parse(message);

	if (jsonObj["payload"]["success"].empty())
	{
		std::cout << "Subscription failure! " << message << "\n";
		CrowdControlRunner::ClearToken();
		SendHello();
		return;
	}

	ServerRequests::RequestGet(L"user/profile", [](const std::wstring &response)
							   {
		nlohmann::json streamerJSON = nlohmann::json::parse(response);
		std::wcout << L"Received response: " << response << std::endl;

		streamer = std::make_shared<StreamUser>();
		streamer->Streamer(streamerJSON);

		CrowdControlRunner::commandCode = 3;

		if (startSessionAutomatically) {
			CrowdControlRunner::StartGameSession();
		} });
}

std::string DisplayNameToID(std::string displayName)
{
	for (const auto &pair : CrowdControlRunner::effects)
	{
		if (pair.second->displayName == displayName)
		{
			return pair.second->id;
		}
	}

	return "";
}

bool CrowdControlRunner::IsRunning(std::string displayName)
{
	std::string id = DisplayNameToID(displayName);
	return CrowdControlRunner::runningEffects.count(id) > 0 && !CrowdControlRunner::runningEffects[id]->timedEffect->paused;
}

bool CrowdControlRunner::IsPaused(std::string displayName)
{
	std::string id = DisplayNameToID(displayName);
	return CrowdControlRunner::runningEffects.count(id) > 0 && CrowdControlRunner::runningEffects[id]->timedEffect->paused;
}

bool CrowdControlRunner::PauseEffect(std::string displayName)
{
	if (!CrowdControlRunner::IsRunning(displayName))
	{
		return false;
	}

	std::string id = DisplayNameToID(displayName);

	if (!runningEffects[id]->timedEffect->paused)
	{
		runningEffects[id]->Pause();
		return true;
	}

	return false;
}

bool CrowdControlRunner::ResumeEffect(std::string displayName)
{
	if (!CrowdControlRunner::IsRunning(displayName))
	{
		return false;
	}

	std::string id = DisplayNameToID(displayName);

	if (runningEffects[id]->timedEffect->paused)
	{
		runningEffects[id]->Resume();
		return true;
	}

	return false;
}

bool CrowdControlRunner::ResetEffect(std::string displayName)
{
	if (!CrowdControlRunner::IsRunning(displayName))
	{
		return false;
	}

	std::string id = DisplayNameToID(displayName);

	runningEffects[id]->Reset();

	return true;
}

bool CrowdControlRunner::StopEffect(std::string displayName)
{
	if (!CrowdControlRunner::IsRunning(displayName))
	{
		return false;
	}

	std::string id = DisplayNameToID(displayName);

	if (haltedTimers.find(id) != haltedTimers.end() && !haltedTimers[id].empty())
	{
		RPC::TimedEnd(*CrowdControlRunner::runningEffects[id]);
		CrowdControlRunner::runningEffects[id] = haltedTimers[id].front();
		haltedTimers[id].pop();
		RPC::Success(*CrowdControlRunner::runningEffects[id].get());
		CrowdControlRunner::ResetEffect(displayName);
	}
	else
	{
		runningEffects[id]->Stop(false);
		runningEffects.erase(id);
	}

	return true;
}

void CrowdControlRunner::StopAllEffects()
{
	for (const auto &runningTimedEffect : CrowdControlRunner::runningEffects)
	{
		std::string effectID = runningTimedEffect.second->effect->id;
		runningEffects[effectID]->Stop(true);
	}

	for (auto &pair : haltedTimers)
	{
		std::queue<std::shared_ptr<CCEffectInstanceTimed>> &queue = pair.second;
		while (!queue.empty())
		{
			std::shared_ptr<CCEffectInstanceTimed> instance = queue.front();
			RPC::FailTemporarily(*instance.get());
			queue.pop();
		}
	}

	while (!pendingQueue.empty())
	{
		std::shared_ptr<CCEffectInstance> effectInstance = pendingQueue.front();
		RPC::FailTemporarily(*effectInstance.get());
		pendingQueue.pop();
	}

	haltedTimers.clear();
}

void Subscribe()
{
	nlohmann::json subObj;
	subObj["action"] = "subscribe";

	nlohmann::json subContents;
	subContents["token"] = CrowdControlRunner::token;
	subContents["topics"] = {"session/self", "prv/self", "pub/self"};

	subObj["data"] = std::string(subContents.dump());

	CrowdControlRunner::WriteToSocket(subObj);
}

void Login(std::string loginPlatform)
{
	std::string url = "https://auth.crowdcontrol.live?platform=" + loginPlatform + "&connectionID=" + CrowdControlRunner::connectionID;
	std::string command = "start \"\" \"" + url + "\"";
	std::system(command.c_str());
}

void CrowdControlRunner::LoginTwitch()
{
	Login("twitch");
}

void CrowdControlRunner::LoginYoutube()
{
	Login("youtube");
}

void CrowdControlRunner::LoginDiscord()
{
	Login("discord");
}

std::string ToLower(std::string str)
{
	for (char &c : str)
	{
		c = std::tolower(static_cast<unsigned char>(c));
	}

	return str;
}

void CrowdControlRunner::SaveToken()
{
	std::ofstream outFile("token.cc");
	outFile.close();
}

void CrowdControlRunner::ClearToken()
{
	CrowdControlRunner::token = "";
	SaveToken();
}

void CrowdControlRunner::ChooseSite()
{
	std::string loginPlatform = "";

	while (loginPlatform.empty())
	{
		std::cout << "Please login to either Twitch, Youtube or Discord: ";
		std::cin >> loginPlatform;
		loginPlatform = ToLower(loginPlatform);

		if (loginPlatform != "twitch" && loginPlatform != "youtube" && loginPlatform != "discord")
		{
			std::cout << "\nInvalid platform.\n";
			loginPlatform = "";
		}
	}

	Login(loginPlatform);
}

void Processwhoami(std::string message)
{
	nlohmann::json jsonObj = nlohmann::json::parse(message);
	CrowdControlRunner::connectionID = jsonObj["payload"]["connectionID"];
	std::ifstream file("token.cc");

	if (file.good())
	{
		std::ifstream inFile("token.cc");
		std::getline(inFile, CrowdControlRunner::token);
		inFile.close();
	}

	if (CrowdControlRunner::token == "")
	{
		CrowdControlRunner::commandCode = 2;

		if (CrowdControlRunner::engine == "")
		{
			CrowdControlRunner::ChooseSite();
		}
	}
	else
	{
		Subscribe();
	}
}

void ProcessLoginSuccess(std::string message)
{
	nlohmann::json jsonObj = nlohmann::json::parse(message);
	CrowdControlRunner::token = jsonObj["payload"]["token"];
	CrowdControlRunner::SaveToken();
	Subscribe();
}

std::shared_ptr<CCEffectBase> GetEffect(const std::string &key)
{
	auto it = CrowdControlRunner::effects.find(key);
	if (it != CrowdControlRunner::effects.end())
	{
		return it->second;
	}
	// Return nullptr or handle the case where the key is not found
	return nullptr;
}

std::shared_ptr<StreamUser> GetUser(const std::string &key)
{
	if (key == "Local")
	{
		return localUser;
	}

	auto it = streamUsers.find(key);
	if (it != streamUsers.end())
	{
		return it->second;
	}

	// Return nullptr or handle the case where the key is not found
	return 0;
}

void ReceiveEffectRequest(nlohmann::json payload, bool test)
{
	if (std::find(effectInstanceIDs.begin(), effectInstanceIDs.end(), payload["requestID"]) != effectInstanceIDs.end())
	{
		return;
	}

	std::shared_ptr<CCEffectBase> effect = GetEffect(payload["effect"]["effectID"].get<std::string>());

	if (effect == nullptr)
	{
		Streambuf::Important("Received a bad effect: " + payload["effect"]["effectID"]);
		return;
	}

	std::shared_ptr<StreamUser> sender = GetUser(payload["requester"]["name"].get<std::string>());

	if (sender == 0)
	{
		sender = std::make_shared<StreamUser>();
		sender->StreamUserFromEffect(payload);
		streamUsers[sender->name] = sender;
	}

	std::shared_ptr<CCEffectInstance> instance;

	CCEffectTimed *timedEffect = dynamic_cast<CCEffectTimed *>(effect.get());
	CCEffectParameters *paramEffect = dynamic_cast<CCEffectParameters *>(effect.get());

	if (timedEffect)
	{
		// Assuming CreateEffectInstance returns a pointer to CCEffectInstanceTimed
		auto instanceTimed = std::shared_ptr<CCEffectInstanceTimed>(CreateEffectInstance<CCEffectInstanceTimed>(effect, sender, payload["requestID"]));
		instanceTimed->runTime = payload["effect"]["duration"].get<float>();
		instanceTimed->timedEffect = timedEffect;
		instance = instanceTimed;
	}
	else if (paramEffect)
	{
		auto instanceParam = std::shared_ptr<CCEffectInstanceParameters>(CreateEffectInstance<CCEffectInstanceParameters>(effect, sender, payload["requestID"]));

		instanceParam->quantity = payload["quantity"];

		nlohmann::json parameters = payload["parameters"];

		for (nlohmann::json::iterator it = parameters.begin(); it != parameters.end(); ++it)
		{
			nlohmann::json parameterEntry = parameters[it.key()];
			instanceParam->SetParam(parameterEntry["title"], parameterEntry["name"]);
		}

		instance = instanceParam;
	}
	else
	{
		instance = std::shared_ptr<CCEffectInstance>(CreateEffectInstance<CCEffectInstance>(effect, sender, payload["requestID"]));
	}

	effectInstanceIDs.push_back(payload["requestID"]);

	if (effectInstanceIDs.size() == 10)
	{
		effectInstanceIDs.erase(effectInstanceIDs.begin());
	}

	std::shared_ptr<CCEffectInstance> ptr = instance;
	pendingQueue.push(ptr);
}

void ProcessJSONMessage(std::string message)
{
	std::cout << "RECEIVED: " << message << "\n";
	nlohmann::json jsonObj = nlohmann::json::parse(message);
	std::string messageType = jsonObj["type"];

	std::wstring payloadWStr;
	nlohmann::json payload = nullptr;

	if (jsonObj.contains("payload"))
	{
		auto &payloadVal = jsonObj["payload"];
		std::string payloadStr = payloadVal.dump();
		payloadWStr.assign(payloadStr.begin(), payloadStr.end());
		payload = jsonObj["payload"];
	}

	if (messageType == "whoami")
	{
		Processwhoami(message);
	}
	else if (messageType == "login-success")
	{
		ProcessLoginSuccess(message);
	}
	else if (messageType == "subscription-result")
	{
		ProcessSubResult(message);
	}
	else if (messageType == "game-session-start")
	{
		StartGameSessionProcess(payloadWStr);
	}
	else if (messageType == "game-session-stop")
	{
		StopGameSessionProcess(payloadWStr);
	}
	else if (messageType == "effect-request")
	{
		ReceiveEffectRequest(payload, false);
	}
}

void CrowdControlRunner::Success(char *id)
{
	RPC::Success(std::string(id), 0, 0);
}

void CrowdControlRunner::Fail(char *id)
{
	RPC::FailTemporarily(std::string(id), 0, 0);
}

void DoRead()
{
	if (GetMillisecondsSinceOffset(update_time) < (CrowdControlRunner::FPS / 1000))
	{
		return;
	}

	update_time = std::chrono::steady_clock::now();

	if (ccSocket->next_layer().next_layer().available() > 0)
	{
		beast::flat_buffer buffer;
		ccSocket->read(buffer);
		std::string gotData = beast::buffers_to_string(buffer.data());
		ProcessJSONMessage(gotData);
	}

	if (!PostGetResponses.empty())
	{
		auto responseData = PostGetResponses.front();
		PostGetResponses.pop();
		std::wcout << "RECEIVED: " << responseData.second << "\n";
		responseData.first(responseData.second);
	}

	if (GetMillisecondsSinceOffset(effect_delay) >= 3000 && !pendingQueue.empty())
	{
		effect_delay = std::chrono::steady_clock::now();

		std::shared_ptr<CCEffectInstance> instance = pendingQueue.front();
		pendingQueue.pop();

		std::shared_ptr<CCEffectInstanceTimed> timedInstance = std::dynamic_pointer_cast<CCEffectInstanceTimed>(instance);
		std::shared_ptr<CCEffectInstanceParameters> parameterInstance = std::dynamic_pointer_cast<CCEffectInstanceParameters>(instance);

		if (timedInstance == nullptr && parameterInstance == nullptr)
		{
		}

		engineQueuedEffects.push(instance);

		/*
		EffectResult result = EffectResult::Failure;

		if (!instance->effect->CanBeRan()) {
			result = EffectResult::Failure;
		}
		else {
			if (timedInstance) {
				std::shared_ptr<CCEffectTimed> timedEffect = std::dynamic_pointer_cast<CCEffectTimed>(timedInstance->effect);
				result = timedEffect->OnTriggerEffect(timedInstance.get());
			}
			else if (paramif (parameterInstance) {
				std::shared_ptr<CCEffectParameters> parameterEffect = std::dynamic_pointer_cast<CCEffectParameters>(parameterInstance->effect);
				result = parameterEffect->OnTriggerEffect(parameterInstance.get());
			}
			else {
				result = instance->effect->OnTriggerEffect(instance.get());
			}
		}

		if (result == EffectResult::Success) {
			if (timedInstance) {
				if (CrowdControlRunner::runningEffects.find(instance->effect->id) != CrowdControlRunner::runningEffects.end()) {
					haltedTimers[instance->effect->id].push(timedInstance);
				}
				else {
					timedInstance->startTime = std::chrono::steady_clock::now();
					CrowdControlRunner::runningEffects[instance->effect->id] = timedInstance;
					RPC::Success(*instance.get());
				}
			}
			else {
				RPC::Success(*instance.get());
			}
		}
		else if (result == EffectResult::Failure) {
			RPC::FailTemporarily(*instance.get());
		}
		else if (result == EffectResult::Unavailable) {
			RPC::FailPermanently(*instance.get());
		}
		else if (result == EffectResult::Retry) {
			instance->retryCount++;

			if (instance->retryCount >= maxRetries) {
				RPC::FailTemporarily(*instance.get());
				return;
			}

			instance->unscaledStartTime = GetMillisecondsSinceOffset(program_start);
			pendingQueue.push(instance);
		}*/
	}
	/*
	for (const auto& runningTimedEffect : CrowdControlRunner::runningEffects) {
		std::string effectID = runningTimedEffect.second->effect->id;

		if (CrowdControlRunner::runningEffects[effectID]->timedEffect->paused || !CrowdControlRunner::runningEffects[effectID]->timedEffect->RunningCondition()) {
			continue;
		}

		std::cout << runningTimedEffect.second->TimeRemaining() << "\n";

		CrowdControlRunner::runningEffects[effectID]->timedEffect->OnUpdate();

		if (runningTimedEffect.second->TimeRemaining() <= 0) {
			CrowdControlRunner::StopEffect(runningTimedEffect.second->effect->displayName);
			break;
		}
	}*/
}

void AddEffect(std::shared_ptr<CCEffectBase> effect)
{
	CrowdControlRunner::effects[effect->id] = effect;
	effectIDs.push_back(effect->id);
}

bool CrowdControlRunner::HasRunningEffects()
{
	return CrowdControlRunner::runningEffects.size() > 0;
}

std::unique_ptr<websocket::stream<beast::ssl_stream<tcp::socket>>> ws_ptr;
net::io_context ioc;
ssl::context ctx{ssl::context::tlsv12_client};

void CrowdControlRunner::Connect()
{
	std::string host = "pubsub.crowdcontrol.live";
	auto const port = "443";

	ctx.set_options(ssl::context::default_workarounds | ssl::context::no_sslv2 | ssl::context::no_sslv3 | ssl::context::no_tlsv1 | ssl::context::no_tlsv1_1 | ssl::context::single_dh_use);

	tcp::resolver resolver{ioc};

	ws_ptr = std::make_unique<websocket::stream<beast::ssl_stream<tcp::socket>>>(ioc, ctx);
	ccSocket = ws_ptr.get();

	auto const results = resolver.resolve(host, port);
	auto ep = net::connect(get_lowest_layer(*ccSocket), results);

	if (!SSL_set_tlsext_host_name(ccSocket->next_layer().native_handle(), host.c_str()))
		throw beast::system_error(
			beast::error_code(
				static_cast<int>(::ERR_get_error()),
				net::error::get_ssl_category()),
			"Failed to set SNI Hostname");

	host += ':' + std::to_string(ep.port());
	ccSocket->next_layer().handshake(ssl::stream_base::client);

	ccSocket->set_option(websocket::stream_base::decorator(
		[](websocket::request_type &req)
		{
			req.set(http::field::user_agent,
					std::string(BOOST_BEAST_VERSION_STRING) +
						" websocket-client-coro");
		}));

	ccSocket->handshake(host, "/");

	CrowdControlRunner::connected = true;

	SendHello();

	while (CrowdControlRunner::connected)
	{
		DoRead();
	}
}

void CrowdControlRunner::Disconnect()
{
	CrowdControlRunner::StopAllEffects();
	CrowdControlRunner::StopGameSession();
	CrowdControlRunner::connected = false;
	ccSocket->close(websocket::close_code::normal);
	CrowdControlRunner::commandCode = 1;
}

std::string CrowdControlRunner::JSONManifest()
{
	nlohmann::json effectsManifest;

	for (const auto &effect : CrowdControlRunner::effects)
	{
		effectsManifest[effect.first] = effect.second->JSONManifest();
	}

	nlohmann::json manifest;
	manifest["meta"]["effects"]["game"] = effectsManifest;

	manifest["meta"]["patch"] = FALSE;
	manifest["meta"]["name"] = CrowdControlRunner::gameName;
	manifest["meta"]["platform"] = "PC";

	return std::string(manifest.dump());
}

void CrowdControlRunner::TestEffect(std::string displayName, std::map<std::string, std::string> paramPairs)
{
	nlohmann::json testPayload;

	std::string id = DisplayNameToID(displayName);

	testPayload["effect"]["effectID"] = id;
	testPayload["requester"]["name"] = "Local";

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<std::uint32_t> dis(0, 2000000000);

	std::uint32_t randomNumber = dis(gen);

	testPayload["requestID"] = std::to_string(randomNumber);

	std::shared_ptr<CCEffectBase> effect = GetEffect(id);

	CCEffectTimed *timedEffect = dynamic_cast<CCEffectTimed *>(effect.get());
	CCEffectParameters *paramEffect = dynamic_cast<CCEffectParameters *>(effect.get());

	if (timedEffect)
	{
		testPayload["effect"]["duration"] = timedEffect->duration;
	}
	else if (paramEffect)
	{
		for (const auto pair : paramPairs)
		{
			std::string key = pair.first;
			std::string value = pair.second;

			if (key == "_quantity")
			{
				int quantityValue = std::stoi(value);
				testPayload["quantity"] = quantityValue;
			}
			else
			{
				testPayload["parameters"][key]["title"] = key;
				testPayload["parameters"][key]["name"] = value;
			}
		}
	}

	ReceiveEffectRequest(testPayload, true);
}

void EffectRequestProcess(const std::wstring &response)
{
	nlohmann::json testJson = nlohmann::json::parse(response);
	ReceiveEffectRequest(testJson["effectRequest"], true);
}

void CrowdControlRunner::TestEffectRemotely(std::string displayName, std::map<std::string, std::string> paramPairs)
{
	std::string id = DisplayNameToID(displayName);

	nlohmann::json testMessage;
	testMessage["gameSessionID"] = CrowdControlRunner::gameSessionID;
	testMessage["sourceDetails"]["type"] = "crowd-control-test";
	testMessage["effectType"] = "game";
	testMessage["effectID"] = id;

	for (const auto pair : paramPairs)
	{
		std::string key = pair.first;
		std::string value = pair.second;

		/*if (key == "_quantity") {
			int quantityValue = std::stoi(value);
			testPayload["quantity"] = quantityValue;
		}
		else {
			testPayload["parameters"][key]["title"] = key;
			testPayload["parameters"][key]["name"] = value;
		}*/
	}

	web::json::value webJson = web::json::value::parse(testMessage.dump());

	ServerRequests::SendPost(L"effect-request", EffectRequestProcess, webJson, true);
}

int CrowdControlRunner::CommandID()
{
	return CrowdControlRunner::commandCode;
}

void CrowdControlRunner::EngineSet()
{
	CrowdControlRunner::engine = "Engine";
}

void CrowdControlRunner::ResetCommandCode()
{
	CrowdControlRunner::commandCode = 0;
}

char *CrowdControlRunner::TestCharArray()
{
	char *charArray = new char[2000];

	if (!Streambuf::queue.empty())
	{
		charArray = new char[2000];
		std::string tempStr = Streambuf::queue.front();
		Streambuf::queue.pop();
		strcpy_s(charArray, 2000, tempStr.c_str());
	}
	else
	{
		charArray[0] = '\0';
		charArray[1] = '\0';
	}

	return charArray;
}

char *CrowdControlRunner::EngineEffect()
{
	char *charArray = new char[2000];

	if (!engineQueuedEffects.empty())
	{
		charArray = new char[2000];

		std::shared_ptr<CCEffectInstance> effect = engineQueuedEffects.front();
		engineQueuedEffects.pop();

		nlohmann::json effectManifest;
		effectManifest["name"] = effect->effect->displayName;
		effectManifest["id"] = effect->id;

		std::shared_ptr<CCEffectInstanceTimed> timedInstance = std::dynamic_pointer_cast<CCEffectInstanceTimed>(effect);
		std::shared_ptr<CCEffectInstanceParameters> parameterInstance = std::dynamic_pointer_cast<CCEffectInstanceParameters>(effect);

		if (parameterInstance != nullptr) {
			if (parameterInstance->parameters.size() > 0)
			{
				effectManifest["value"] = parameterInstance->parameters.begin()->second;
			}
			else
			{
				effectManifest["value"] = parameterInstance->quantity;
			}
		}
		else if (timedInstance)
		{
			effectManifest["duration"] = std::to_string(timedInstance->TimeRemaining());
		}



		std::string jsonString = effectManifest.dump(); // Convert JSON object to string

		strcpy_s(charArray, 2000, jsonString.c_str());
	}
	else
	{
		charArray[0] = '\0';
		charArray[1] = '\0';
	}

	return charArray;
}

void CrowdControlRunner::AddBasicEffect(char *name, char *desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char **categoriesArray)
{
	std::shared_ptr<CCEffectBase> effect = std::make_shared<CCEffectTest>();
	effect->Setup(name, desc, price, retries, retryDelay, pendingDelay, sellable, visible, nonPoolable, morality, orderliness, categoriesArray);
	std::cout << "Added Effect " << effect->displayName;
	AddEffect(effect);
}

void CrowdControlRunner::AddTimedEffect(char *name, char *desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char **categoriesArray, float duration)
{
	std::shared_ptr<CCEffectTimed> timedEffect = std::make_shared<CCEffectTimedTest>();
	timedEffect->SetupTimed(name, desc, price, retries, retryDelay, pendingDelay, sellable, visible, nonPoolable, morality, orderliness, categoriesArray, duration);
	std::cout << "Added Timed Effect " << timedEffect->displayName;
	AddEffect(timedEffect);
}

void CrowdControlRunner::AddParameterEffect(char *name, char *desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char **categoriesArray)
{
	std::shared_ptr<CCEffectParameters> effect = std::make_shared<CCEffectParametersTest>();
	effect->Setup(name, desc, price, retries, retryDelay, pendingDelay, sellable, visible, nonPoolable, morality, orderliness, categoriesArray);
	std::cout << "Added Parameter Effect " << effect->displayName;
	AddEffect(effect);
}

void CrowdControlRunner::AddParameterOption(char *name, char *paramName, char **options)
{
	std::string effectID = DisplayNameToID(name);
	std::shared_ptr<CCEffectBase> effect = CrowdControlRunner::effects[effectID];
	std::shared_ptr<CCEffectParameters> effectParameters = std::dynamic_pointer_cast<CCEffectParameters>(effect);

	effectParameters->AddOptionsParameter(paramName, options);
}

void CrowdControlRunner::AddParameterMinMax(char *name, char *paramName, int min, int max)
{
	std::string effectID = DisplayNameToID(name);
	std::shared_ptr<CCEffectBase> effect = CrowdControlRunner::effects[effectID];
	std::shared_ptr<CCEffectParameters> effectParameters = std::dynamic_pointer_cast<CCEffectParameters>(effect);

	effectParameters->AddMinMaxParameter(paramName, min, max);
}

void CrowdControlRunner::SetGameNameAndPackID(char* name, char* packID)
{
	gamePackID = std::string(packID);
	gameName = std::string(name);
}

// Sends a WebSocket message and prints the response
int CrowdControlRunner::Run()
{
	localUser = std::make_shared<StreamUser>();
	localUser->LocalUser();

	std::cout.rdbuf(&customBuf);

	//gamePackID = "UnityDemo";
	//gameName = "Unity Demo";

	std::cout << CrowdControlRunner::JSONManifest();

	try
	{
		CrowdControlRunner::Connect();
	}
	catch (std::exception const &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
