#include "CCEffectParameters.hpp"
#include "CCEffectParametersTest.hpp"
#include "StreamBuf.hpp"
#include <iostream>

void CCEffectParametersTest::SetupParams() {
	
}

EffectResult CCEffectParametersTest::OnTriggerEffect(CCEffectInstanceParameters* effectInstance) {
	//std::cout << effectInstance->quantity << "\n";
	//std::cout << effectInstance->GetParam("Total Coins") << "\n";
	return EffectResult::Success;
}