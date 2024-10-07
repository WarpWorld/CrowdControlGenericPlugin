#include "include/StreamUser.hpp"

StreamUser::StreamUser() {

}

void StreamUser::Streamer(nlohmann::json json) {

}

StreamUser::StreamUser(nlohmann::json json) {
	originSite = json["profile"]["type"];
	name = json["profile"]["name"];
	displayName = json["profile"]["originData"]["user"]["display_name"];
	email = json["profile"]["originData"]["user"]["email"];
	profileIconUrl = json["profile"]["image"];
	roles = json["profile"]["roles"].get<std::vector<std::string>>();
	subscriptions = json["profile"]["subscriptions"].get<std::vector<std::string>>();
	originID = json["profile"]["originID"];
}

void StreamUser::StreamUserFromEffect(nlohmann::json json) {
	originSite = json["requester"]["profile"];
	name = json["requester"]["name"];
	displayName = json["requester"]["name"];
	originID = json["requester"]["originID"];
	profileIconUrl = json["requester"]["image"];

	if (json["requester"].contains("roles") && json["requester"]["roles"].is_array()) {
		roles = json["requester"]["roles"].get<std::vector<std::string>>();
	}

	if (json["requester"].contains("subscriptions") && json["requester"]["subscriptions"].is_array()) {
		subscriptions = json["requester"]["subscriptions"].get<std::vector<std::string>>();
	}
}

void StreamUser::LocalUser() {
	originSite = "Local";
	name = "Local";
	displayName = "Local";
	originID = "0";
}
