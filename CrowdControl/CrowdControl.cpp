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

#include "crowdcontrol.hpp"
#include "ServerRequests.hpp"
#include "StreamUser.hpp"

#include "CCEffectBase.hpp"
#include "CCEffect.hpp"
#include "CCEffectTimed.hpp"
#include "CCEffectParameters.hpp"
#include "CCEffectInstanceParameters.hpp"
#include "CCEffectTest.hpp"
#include "CCEffectTimedTest.hpp"
#include "CCEffectParametersTest.hpp"
#include "EffectResult.hpp"
#include "RPC.hpp"
#include "StreamBuf.hpp"

namespace beast = boost::beast;      
namespace http = beast::http;  
namespace websocket = beast::websocket; 
namespace net = boost::asio;    
namespace ssl = boost::asio::ssl; 
using tcp = boost::asio::ip::tcp;

websocket::stream<beast::ssl_stream<tcp::socket>>* ccSocket = nullptr;

std::string CrowdControl::connectionID;
std::string CrowdControl::token;
std::string CrowdControl::gamePackID;
std::string CrowdControl::gameName;
std::string CrowdControl::gameSessionID;
bool CrowdControl::connected;
std::shared_ptr<StreamUser> streamer;
bool startSessionAutomatically = true;
int maxRetries = 3;
bool CrowdControl::sendingPost = false;

static std::queue<std::pair<std::function<void(const std::wstring&)>, std::wstring>> PostGetResponses;
std::unordered_map<std::string, std::shared_ptr<CCEffectBase>> CrowdControl::effects;
std::unordered_map<std::string, std::shared_ptr<StreamUser>> streamUsers;
std::unordered_map<std::string, std::shared_ptr<CCEffectInstanceTimed>> CrowdControl::runningEffects;
std::unordered_map<std::string, std::queue<std::shared_ptr<CCEffectInstanceTimed>>> haltedTimers;

std::vector<std::string> effectIDs;
std::vector<std::string> effectInstanceIDs;
std::queue<std::shared_ptr<CCEffectInstance>> pendingQueue;

std::chrono::steady_clock::time_point program_start = std::chrono::steady_clock::now();
std::chrono::steady_clock::time_point effect_delay = std::chrono::steady_clock::now();
std::chrono::steady_clock::time_point update_time = std::chrono::steady_clock::now();

std::shared_ptr<StreamUser> localUser;

CrowdControl::CrowdControl() {

}

CrowdControl::~CrowdControl() {
	CrowdControl::StopAllEffects();
	CrowdControl::StopGameSession();
}

void CrowdControl::PushToQueue(const std::function<void(const std::wstring&)>& callback, const std::wstring& message) {
	PostGetResponses.push({ callback, message });
}

std::string ToLower(std::string str) {
	for (char& c : str) {
		c = std::tolower(static_cast<unsigned char>(c));
	}

	return str;
}

void CrowdControl::SaveToken() {
	std::ofstream outFile("token.cc");
	outFile.close();
}

void CrowdControl::ClearToken() {
	CrowdControl::token = "";
	SaveToken();
}

void CrowdControl::WriteToSocket(nlohmann::json jsonObj) {
	std::cout << "SENT: " + jsonObj.dump() << "\n";
	ccSocket->write(net::buffer(std::string(jsonObj.dump())));
}

void Subscribe() {
	nlohmann::json subObj;
	subObj["action"] = "subscribe";

	nlohmann::json subContents;
	subContents["token"] = CrowdControl::token;
	subContents["topics"] = { "session/self", "prv/self", "pub/self" };

	subObj["data"] = std::string(subContents.dump());

	CrowdControl::WriteToSocket(subObj);
}

void Login(std::string loginPlatform) {
	std::string url = "https://auth.crowdcontrol.live?platform="
		+ loginPlatform
		+ "&connectionID="
		+ CrowdControl::connectionID;
	std::string command = "start \"\" \"" + url + "\"";
	std::system(command.c_str());
}

void CrowdControl::LoginTwitch() {
	Login("twitch");
}

void CrowdControl::LoginYoutube() {
	Login("youtube");
}

void CrowdControl::LoginDiscord() {
	Login("discord");
}

void Processwhoami(std::string message) {
	nlohmann::json jsonObj = nlohmann::json::parse(message);
	CrowdControl::connectionID = jsonObj["payload"]["connectionID"];

	std::ifstream file("token.cc");

	if (file.good()) {
		std::ifstream inFile("token.cc");
		std::getline(inFile, CrowdControl::token);
		inFile.close();
	}

	if (CrowdControl::token.empty()) {
		std::string loginPlatform = "";

		while (loginPlatform.empty()) {
			std::cout << "Please login to either Twitch, Youtube or Discord: ";
			std::cin >> loginPlatform;
			loginPlatform = ToLower(loginPlatform);

			if (loginPlatform != "twitch" && loginPlatform != "youtube" && loginPlatform != "discord") {
				std::cout << "\nInvalid platform.\n";
				loginPlatform = "";
			}
		}

		Login(loginPlatform);
	}
	else
	{
		Subscribe();
	}
}

void ProcessLoginSuccess(std::string message) {
	nlohmann::json jsonObj = nlohmann::json::parse(message);
	CrowdControl::token = jsonObj["payload"]["token"];
	CrowdControl::SaveToken();
	Subscribe();
}

void SendHello() {
	nlohmann::json helloObj;
	helloObj["action"] = "whoami";
	CrowdControl::WriteToSocket(helloObj);
}

void StopGameSessionProcess(const std::wstring& response) {
	if (CrowdControl::gameSessionID.empty()) {
		return;
	}

	CrowdControl::gameSessionID = "";
	std::cout << "Stopped the Crowd Control Session!" << "\n";
}

void StartGameSessionProcess(const std::wstring& response) {
	if (ServerRequests::makingRequest) {
		return;
	}

	nlohmann::json startJSON = nlohmann::json::parse(response);
	CrowdControl::gameSessionID = startJSON["gameSessionID"];
	Streambuf::Important("Started the Crowd Control Session!");
}

void CrowdControl::StopGameSession() {
	nlohmann::json stopMessage;
	stopMessage["gamePackID"] = CrowdControl::gamePackID;
	web::json::value webJson = web::json::value::parse(stopMessage.dump());
	ServerRequests::SendPost(L"stop", StartGameSessionProcess, webJson, true);
}

void CrowdControl::StartGameSession() {
	nlohmann::json startMessage;
	startMessage["gamePackID"] = CrowdControl::gamePackID;
	web::json::value webJson = web::json::value::parse(startMessage.dump());
	ServerRequests::SendPost(L"start", StartGameSessionProcess, webJson, true);
}

void ProcessSubResult(std::string message) {
	nlohmann::json jsonObj = nlohmann::json::parse(message);

	if (jsonObj["payload"]["success"].empty()) {
		std::cout << "Subscription failure! " << message << "\n";
		CrowdControl::ClearToken();
		SendHello();
		return;
	}

	ServerRequests::RequestGet(L"user/profile", [](const std::wstring& response) {
		nlohmann::json streamerJSON = nlohmann::json::parse(response);
		std::wcout << L"Received response: " << response << std::endl;

		streamer = std::make_shared<StreamUser>();
		streamer->Streamer(streamerJSON);

		if (startSessionAutomatically) {
			CrowdControl::StartGameSession();
		}
	});
}

std::shared_ptr<CCEffectBase> GetEffect(const std::string& key) {
	auto it = CrowdControl::effects.find(key);
	if (it != CrowdControl::effects.end()) {
		return it->second;
	}
	// Return nullptr or handle the case where the key is not found
	return nullptr;
}

std::shared_ptr<StreamUser> GetUser(const std::string& key) {
	if (key == "Local") {
		return localUser;
	}

	auto it = streamUsers.find(key);
	if (it != streamUsers.end()) {
		return it->second;
	}

	// Return nullptr or handle the case where the key is not found
	return 0;
}

long GetMillisecondsSinceOffset(std::chrono::steady_clock::time_point offset) {
	auto now = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - offset);
	return duration.count();
}

template<typename T>
std::shared_ptr<T> CreateEffectInstance(std::shared_ptr<CCEffectBase> effect, std::shared_ptr<StreamUser> sender, std::string id) {
	auto instance = std::make_shared<T>(); // Create a shared pointer of type T
	instance->effect = std::dynamic_pointer_cast<CCEffectBase>(effect);
	instance->sender = sender;
	instance->id = id;
	instance->unscaledStartTime = GetMillisecondsSinceOffset(program_start);
	std::cout << "Creating instance of type: " << typeid(T).name() << std::endl;

	return instance;
}

void ReceiveEffectRequest(nlohmann::json payload, bool test) {
	if (std::find(effectInstanceIDs.begin(), effectInstanceIDs.end(), payload["requestID"]) != effectInstanceIDs.end()) {
		return;
	}

	std::shared_ptr<CCEffectBase> effect = GetEffect(payload["effect"]["effectID"].get<std::string>());

	if (effect == nullptr) {
		Streambuf::Important("Received a bad effect: " + payload["effect"]["effectID"]);
		return;
	}

	std::shared_ptr<StreamUser> sender = GetUser(payload["requester"]["name"].get<std::string>());

	if (sender == 0) {
		sender = std::make_shared<StreamUser>();
		sender->StreamUserFromEffect(payload);
		streamUsers[sender->name] = sender;
	}

	std::shared_ptr<CCEffectInstance> instance;

	CCEffectTimed* timedEffect = dynamic_cast<CCEffectTimed*>(effect.get());
	CCEffectParameters* paramEffect = dynamic_cast<CCEffectParameters*>(effect.get());

	if (timedEffect) {
		// Assuming CreateEffectInstance returns a pointer to CCEffectInstanceTimed
		auto instanceTimed = std::shared_ptr<CCEffectInstanceTimed>(CreateEffectInstance<CCEffectInstanceTimed>(effect, sender, payload["requestID"]));
		instanceTimed->runTime = payload["effect"]["duration"].get<float>();
		instanceTimed->timedEffect = timedEffect;
		instance = instanceTimed;
	}
	else if (paramEffect) {
		auto instanceParam = std::shared_ptr<CCEffectInstanceParameters>(CreateEffectInstance<CCEffectInstanceParameters>(effect, sender, payload["requestID"]));

		instanceParam->quantity = payload["quantity"];

		nlohmann::json parameters = payload["parameters"];

		for (nlohmann::json::iterator it = parameters.begin(); it != parameters.end(); ++it) {
			nlohmann::json parameterEntry = parameters[it.key()];
			instanceParam->SetParam(parameterEntry["title"], parameterEntry["name"]);
		}

		instance = instanceParam;
	}
	else {
		instance = std::shared_ptr<CCEffectInstance>(CreateEffectInstance<CCEffectInstance>(effect, sender, payload["requestID"]));
	}

	effectInstanceIDs.push_back(payload["requestID"]);

	if (effectInstanceIDs.size() == 10) {
		effectInstanceIDs.erase(effectInstanceIDs.begin());
	}

	std::shared_ptr<CCEffectInstance> ptr = instance;
	pendingQueue.push(ptr);
}

std::string DisplayNameToID(std::string displayName) {
	for (const auto& pair : CrowdControl::effects) {
		if (pair.second->displayName == displayName) {
			return pair.second->id;
		}
	}

	return "";
}

void CrowdControl::TestEffect(std::string displayName, std::map<std::string, std::string> paramPairs) {
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

	CCEffectTimed* timedEffect = dynamic_cast<CCEffectTimed*>(effect.get());
	CCEffectParameters* paramEffect = dynamic_cast<CCEffectParameters*>(effect.get());

	if (timedEffect) {
		testPayload["effect"]["duration"] = timedEffect->duration;
	}
	else if (paramEffect) {
		for (const auto pair : paramPairs) {
			std::string key = pair.first;
			std::string value = pair.second;

			if (key == "_quantity") {
				int quantityValue = std::stoi(value);
				testPayload["quantity"] = quantityValue;
			}
			else {
				testPayload["parameters"][key]["title"] = key;
				testPayload["parameters"][key]["name"] = value;
			}
		}
	}

	ReceiveEffectRequest(testPayload, true);
}

void EffectRequestProcess(const std::wstring& response) {
	nlohmann::json testJson = nlohmann::json::parse(response);
	ReceiveEffectRequest(testJson["effectRequest"], true);
}

void CrowdControl::TestEffectRemotely(std::string displayName, std::map<std::string, std::string> paramPairs) {
	std::string id = DisplayNameToID(displayName);

	nlohmann::json testMessage;
	testMessage["gameSessionID"] = CrowdControl::gameSessionID;
	testMessage["sourceDetails"]["type"] = "crowd-control-test";
	testMessage["effectType"] = "game";
	testMessage["effectID"] = id;

	for (const auto pair : paramPairs) {
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

void ProcessJSONMessage(std::string message) {
	std::cout << "RECEIVED: " << message << "\n";

	nlohmann::json jsonObj = nlohmann::json::parse(message);

	std::string messageType = jsonObj["type"];
	std::wstring payloadWStr;
	nlohmann::json payload = nullptr;

	if (jsonObj.contains("payload")) {
		auto& payloadVal = jsonObj["payload"];
		std::string payloadStr = payloadVal.dump();
		payloadWStr.assign(payloadStr.begin(), payloadStr.end());
		payload = jsonObj["payload"];
	}

	if (messageType == "whoami") {
		Processwhoami(message);
	}
	else if (messageType == "login-success") {
		ProcessLoginSuccess(message);
	}
	else if (messageType == "subscription-result") {
		ProcessSubResult(message);
	}
	else if (messageType == "game-session-start") {
		StartGameSessionProcess(payloadWStr);
	}
	else if (messageType == "game-session-stop") {
		StopGameSessionProcess(payloadWStr);
	}
	else if (messageType == "effect-request") {
		ReceiveEffectRequest(payload, false);
	}
}

bool CrowdControl::PauseEffect(std::string displayName) {
	if (!CrowdControl::IsRunning(displayName)) {
		return false;
	}

	std::string id = DisplayNameToID(displayName);

	if (!runningEffects[id]->timedEffect->paused) {
		runningEffects[id]->Pause();
		return true;
	}

	return false;
}

bool CrowdControl::ResumeEffect(std::string displayName) {
	if (!CrowdControl::IsRunning(displayName)) {
		return false;
	}

	std::string id = DisplayNameToID(displayName);

	if (runningEffects[id]->timedEffect->paused) {
		runningEffects[id]->Resume();
		return true;
	}

	return false;
}

bool CrowdControl::ResetEffect(std::string displayName) {
	if (!CrowdControl::IsRunning(displayName)) {
		return false;
	}

	std::string id = DisplayNameToID(displayName);

	runningEffects[id]->Reset();

	return true;
}

bool CrowdControl::StopEffect(std::string displayName) {
	if (!CrowdControl::IsRunning(displayName)) {
		return false;
	}

	std::string id = DisplayNameToID(displayName);

	if (haltedTimers.find(id) != haltedTimers.end() && !haltedTimers[id].empty()) {
		RPC::TimedEnd(*CrowdControl::runningEffects[id]);
		CrowdControl::runningEffects[id] = haltedTimers[id].front();
		haltedTimers[id].pop();
		RPC::Success(*CrowdControl::runningEffects[id].get());
		CrowdControl::ResetEffect(displayName);
	}
	else {
		runningEffects[id]->Stop(false);
		runningEffects.erase(id);
	}

	return true;
}

void CrowdControl::StopAllEffects() {
	for (const auto& runningTimedEffect : CrowdControl::runningEffects) {
		std::string effectID = runningTimedEffect.second->effect->id;
		runningEffects[effectID]->Stop(true);
	}

	for (auto& pair : haltedTimers) {
		std::queue<std::shared_ptr<CCEffectInstanceTimed>>& queue = pair.second;
		while (!queue.empty()) {
			std::shared_ptr<CCEffectInstanceTimed> instance = queue.front();
			RPC::FailTemporarily(*instance.get());
			queue.pop();
		}
	}

	while (!pendingQueue.empty()) {
		std::shared_ptr<CCEffectInstance> effectInstance = pendingQueue.front();
		RPC::FailTemporarily(*effectInstance.get());
		pendingQueue.pop();
	}

	haltedTimers.clear();
}

void DoRead() {
	if (GetMillisecondsSinceOffset(update_time) < (CrowdControl::FPS / 1000)) {
		return;
	}

	update_time = std::chrono::steady_clock::now();
	char key;

	if (_kbhit()) {  // Check if a key is pressed
		key = _getch();

		if (key == 'p') {
			CrowdControl::effects["damageplayer"]->ToggleVisible(false);
		}

		if (key == 'r') {
			CrowdControl::ResumeEffect("Invert Controls");
		}

		if (key == 's') {
			CrowdControl::ResetEffect("Invert Controls");
		}
	}

	if (ccSocket->next_layer().next_layer().available() > 0) {
		beast::flat_buffer buffer;
		ccSocket->read(buffer);
		std::string gotData = beast::buffers_to_string(buffer.data());
		ProcessJSONMessage(gotData);
	}
	if (!PostGetResponses.empty()) {
		auto responseData = PostGetResponses.front();
		PostGetResponses.pop();
		std::wcout << "RECEIVED: " << responseData.second << "\n";
		responseData.first(responseData.second);
	}
	if (GetMillisecondsSinceOffset(effect_delay) >= 3000 && !pendingQueue.empty()) {
		effect_delay = std::chrono::steady_clock::now();

		std::shared_ptr<CCEffectInstance> instance = pendingQueue.front();
		pendingQueue.pop();

		std::shared_ptr<CCEffectInstanceTimed> timedInstance = std::dynamic_pointer_cast<CCEffectInstanceTimed>(instance);
		std::shared_ptr<CCEffectInstanceParameters> parameterInstance = std::dynamic_pointer_cast<CCEffectInstanceParameters>(instance);

		EffectResult result = EffectResult::Failure;

		if (!instance->effect->CanBeRan()) {
			result = EffectResult::Failure;
		} 
		else {
			if (timedInstance) {
				std::shared_ptr<CCEffectTimed> timedEffect = std::dynamic_pointer_cast<CCEffectTimed>(timedInstance->effect);
				result = timedEffect->OnTriggerEffect(timedInstance.get());
			}
			else if (parameterInstance) {
				std::shared_ptr<CCEffectParameters> parameterEffect = std::dynamic_pointer_cast<CCEffectParameters>(parameterInstance->effect);
				result = parameterEffect->OnTriggerEffect(parameterInstance.get());
			}
			else {
				result = instance->effect->OnTriggerEffect(instance.get());
			}
		}

		if (result == EffectResult::Success) {
			if (timedInstance) {
				if (CrowdControl::runningEffects.find(instance->effect->id) != CrowdControl::runningEffects.end()) {
					haltedTimers[instance->effect->id].push(timedInstance);
				}
				else {
					timedInstance->startTime = std::chrono::steady_clock::now();
					CrowdControl::runningEffects[instance->effect->id] = timedInstance;
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
		}
	}

	for (const auto& runningTimedEffect : CrowdControl::runningEffects) {
		std::string effectID = runningTimedEffect.second->effect->id;

		if (CrowdControl::runningEffects[effectID]->timedEffect->paused || !CrowdControl::runningEffects[effectID]->timedEffect->RunningCondition()) {
			continue;
		}

		std::cout << runningTimedEffect.second->TimeRemaining() << "\n";

		CrowdControl::runningEffects[effectID]->timedEffect->OnUpdate();

		if (runningTimedEffect.second->TimeRemaining() <= 0) {
			CrowdControl::StopEffect(runningTimedEffect.second->effect->displayName);
			break;
		}
	}
}

std::string CrowdControl::JSONManifest() {
	nlohmann::json effectsManifest;

	for (const auto& effect : CrowdControl::effects) {
		effectsManifest[effect.first] = effect.second->JSONManifest();
	}

	nlohmann::json manifest;
	manifest["meta"]["effects"]["game"] = effectsManifest;

	manifest["meta"]["patch"] = FALSE;
	manifest["meta"]["name"] = CrowdControl::gameName;
	manifest["meta"]["platform"] = "PC";

	return std::string(manifest.dump());
}

void AddEffect(std::shared_ptr<CCEffectBase> effect) {
	CrowdControl::effects[effect->id] = effect;
	effectIDs.push_back(effect->id);
}

bool CrowdControl::HasRunningEffects() {
	return CrowdControl::runningEffects.size() > 0;
}

bool CrowdControl::IsRunning(std::string displayName) {
	std::string id = DisplayNameToID(displayName);
	return CrowdControl::runningEffects.count(id) > 0 && !CrowdControl::runningEffects[id]->timedEffect->paused;
}

bool CrowdControl::IsPaused(std::string displayName) {
	std::string id = DisplayNameToID(displayName);
	return CrowdControl::runningEffects.count(id) > 0 && CrowdControl::runningEffects[id]->timedEffect->paused;
}

void CrowdControl::Connect() {
	std::string host = "pubsub.crowdcontrol.live";
	auto const  port = "443";

	net::io_context ioc;

	ssl::context ctx{ ssl::context::tlsv12_client };

	ctx.set_options(ssl::context::default_workarounds
		| ssl::context::no_sslv2
		| ssl::context::no_sslv3
		| ssl::context::no_tlsv1
		| ssl::context::no_tlsv1_1
		| ssl::context::single_dh_use);

	tcp::resolver resolver{ ioc };

	websocket::stream<beast::ssl_stream<tcp::socket>> ws{ ioc, ctx };
	ccSocket = &ws;

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
		[](websocket::request_type& req)
	{
		req.set(http::field::user_agent,
			std::string(BOOST_BEAST_VERSION_STRING) +
			" websocket-client-coro");
	}));

	ccSocket->handshake(host, "/");

	connected = true;

	SendHello();

	while (connected) {
		DoRead();
	}
}

void CrowdControl::Disconnect() {
	CrowdControl::StopAllEffects();
	CrowdControl::StopGameSession();

	ccSocket->close(websocket::close_code::normal);
}

// Sends a WebSocket message and prints the response
int CrowdControl::Run() {
	localUser = std::make_shared<StreamUser>();
	localUser->LocalUser();

	std::shared_ptr<CCEffectBase> effect = std::make_shared<CCEffectTest>();
	effect->AssignName("Damage Player");

	std::shared_ptr<CCEffectTimed> timedEffect = std::make_shared<CCEffectTimedTest>();
	timedEffect->AssignName("Invert Controls");

	std::shared_ptr<CCEffectParameters> parameterEffect = std::make_shared<CCEffectParametersTest>();
	parameterEffect->AssignName("Give Coins");
	parameterEffect->SetupParams();

	Streambuf customBuf;
	auto* oldBuf = std::cout.rdbuf(&customBuf);
	AddEffect(effect);
	AddEffect(timedEffect);
	AddEffect(parameterEffect);

	gamePackID = "UnityDemo";
	gameName = "Unity Demo";

	std::cout << CrowdControl::JSONManifest();

	try {
		CrowdControl::Connect();
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		std::cout.rdbuf(oldBuf);
		return EXIT_FAILURE;
	}
	std::cout.rdbuf(oldBuf);

	return EXIT_SUCCESS;
}


