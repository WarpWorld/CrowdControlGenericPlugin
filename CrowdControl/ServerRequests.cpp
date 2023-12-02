#include "ServerRequests.hpp"
#include "CrowdControlRunner.hpp"

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/http_listener.h>  
#include <cpprest/json.h>  
#include <cpprest/uri.h>  
#include <cpprest/ws_client.h>  
#include <cpprest/containerstream.h>  
#include <cpprest/interopstream.h>  
#include <cpprest/rawptrstream.h>  
#include <cpprest/producerconsumerstream.h>
#include <iostream>
#include <cpprest/http_client.h>
#include <queue>
#include <utility>

using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency::streams;

bool ServerRequests::makingRequest = false;

pplx::task<void> ServerRequests::SendPost(const std::wstring& postType, std::function<void(const std::wstring&)> callback, web::json::value json, bool gameSession) {
	std::wstring url;

	if (gameSession) {
		url = L"https://openapi.crowdcontrol.live/game-session/" + postType;
	}
	else {
		url = L"https://openapi.crowdcontrol.live/" + postType;
	}

	http_client client(url);

	http_request request(methods::POST);
	request.headers().add(L"Authorization", L"cc-auth-token " + std::wstring(CrowdControlRunner::token.begin(), CrowdControlRunner::token.end()));
	request.headers().set_content_type(L"application/json"); // Set Content-Type
	request.set_body(json); // Set the JSON payload

	std::wcout << L"POST: " << url;

	makingRequest = true;

	return client.request(request).then([=](http_response response) {
		if (response.status_code() == status_codes::OK) {
			return response.extract_string();
		}
		else {
			std::wcout << L"Request failed with status code: " + std::to_wstring(response.status_code());
			return pplx::task_from_result(std::wstring());
		}
	}).then([=](pplx::task<std::wstring> previousTask) {
		makingRequest = false;
		try {
			auto result = previousTask.get();
			if (!result.empty()) {
				CrowdControlRunner::PushToQueue(callback, std::wstring(result.begin(), result.end()));
			}
		}
		catch (const http_exception& e) {
			if (e.error_code().value() != 0) {
				// Handle error response
				std::wcout << "Error response: " << e.what();
			}
			else {
				std::wcout << "Error: " << e.what();
			}
		}
	});
}

pplx::task<void> ServerRequests::RequestGet(const std::wstring& getType, std::function<void(const std::wstring&)> callback) {
	std::wstring url = L"https://openapi.crowdcontrol.live/" + getType;
	http_client client(url);

	http_request request(methods::GET);
	request.headers().add(L"Authorization", L"cc-auth-token " + std::wstring(CrowdControlRunner::token.begin(), CrowdControlRunner::token.end()));

	std::wcout << L"GET: " << url;

	makingRequest = true;
	return client.request(request).then([=](http_response response) {
		makingRequest = false;
		if (response.status_code() == status_codes::OK) {
			return response.extract_string();
		}
		else {
			std::wcout << L"Request failed with status code: " + std::to_wstring(response.status_code());
			return pplx::task_from_result(std::wstring());
		}
	}).then([=](pplx::task<std::wstring> previousTask) {
		try {
			auto result = previousTask.get();
			if (!result.empty()) {
				CrowdControlRunner::PushToQueue(callback, std::wstring(result.begin(), result.end()));
			}
		}
		catch (const http_exception& e) {
			if (e.error_code().value() != 0) {
				// Handle error response
				std::wcout << "Error response: " << e.what();
			}
			else {
				std::wcout << "Error: " << e.what();
			}
		}
	});
}
