#include "include/CrowdControlRunner.hpp"
#include "include/Morality.hpp"
#include "include/CCEffectBase.hpp"
#include "include/CCEffectInstance.hpp"
#include "include/ServerRequests.hpp"
#include "include/RPC.hpp"
#include <nlohmann/json.hpp>

CCEffectBase::CCEffectBase() {

}

int MoralityValue(int value) {
	switch (value) {
		case 0: return -90;
		case 1: return -60;
		case 2: return -30;
		case 3: return -10;
		case 4: return  0;
		case 5: return 10;
		case 6: return 30;
		case 7: return 60;
		case 8: return 90;
	}

	return 0;
}

void CCEffectBase::Setup(char* n, char* d, int pr, int ret, float retDelay, float penDelay, bool sell, bool vis, bool noPool, int moral, int ord, char** cats) {
	CCEffectBase::displayName = std::string(n);
	CCEffectBase::AssignName(displayName);
	CCEffectBase::description = std::string(d);
	CCEffectBase::price = pr;

	CCEffectBase::retryDelay = retDelay;
	CCEffectBase::pendingDelay = penDelay;
	CCEffectBase::sellable = sell;
	CCEffectBase::visible = vis;
	CCEffectBase::noPooling = noPool;

	CCEffectBase::morality = static_cast<Morality>(MoralityValue(moral));
	CCEffectBase::orderliness = static_cast<Orderliness>(MoralityValue(ord));

	CCEffectBase::categories.clear();

	if (cats != nullptr) {
		for (int i = 0; cats[i] != nullptr; ++i) {
			CCEffectBase::categories.push_back(std::string(cats[i]));
		}
	}
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

	sellableMessage["gamePackID"] = CrowdControlRunner::gamePackID;
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
