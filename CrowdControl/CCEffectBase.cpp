#include "CrowdControl.hpp"
#include "Morality.hpp"
#include "CCEffectBase.hpp"
#include "CCEffectInstance.hpp"
#include "ServerRequests.hpp"
#include "RPC.hpp"
#include <nlohmann/json.hpp>

CCEffectBase::CCEffectBase() {
	
}

void CCEffectBase::ToggleSellable(bool sell) {
	if (sell) {
		RPC::MenuAvailable(*this);
		sellable = true;
		return;
	}

	RPC::MenuUnavailable(*this);
	sellable = false;
}

void CCEffectBase::ToggleVisible(bool vis) {
	if (vis) {
		RPC::MenuVisible(*this);
		visible = true;
		return;
	}

	RPC::MenuHidden(*this);
	visible = false;
}

void CCEffectBase::UpdatePrice(unsigned int newPrice) {
	nlohmann::json sellableMessage;

	sellableMessage["gamePackID"] = CrowdControl::gamePackID;
	sellableMessage["effectOverrides"] = nlohmann::json::array({
		{
			{"price", newPrice},
			{"effectID", id},
			{"type", "game"}
		}
		});
	web::json::value webJson = web::json::value::parse(sellableMessage.dump());

	ServerRequests::SendPost(L"menu/effects", nullptr, webJson, true);
}

/*
void CCEffectBase::UpdateNonPoolable(bool newNonPoolable) {
	// Implementation here
}

void CCEffectBase::UpdateSessionMax(unsigned int newSessionMax) {
	// Implementation here
}*/

bool CCEffectBase::CanBeRan() {
	return true;
}

bool CCEffectBase::HasParameterID(const std::string& id) {
	return false;
}
