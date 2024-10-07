#include "include/CCEffectParameters.hpp"
#include "include/CCEffectParametersTest.hpp"
#include "include/StreamBuf.hpp"
#include <iostream>

void CCEffectParametersTest::SetupParams() {
	
}

EffectResult CCEffectParametersTest::OnTriggerEffect(CCEffectInstanceParameters* effectInstance) {
	//std::cout << effectInstance->quantity << "\n";
	//std::cout << effectInstance->GetParam("Total Coins") << "\n";
	return EffectResult::Success;
}