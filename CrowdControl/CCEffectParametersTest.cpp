#include "CCEffectParameters.hpp"
#include "CCEffectParametersTest.hpp"
#include "StreamBuf.hpp"
#include <iostream>

void CCEffectParametersTest::SetupParams() {
	ParameterEntry entry("Total Coins", id, ParameterEntry::Kind::Item);
	entry.AddOption("One");
	entry.AddOption("Two");
	entry.AddOption("Three");
	entry.AddOption("Four");
	entry.AddOption("Five");

	parameterEntries[entry.id] = entry;
} 

EffectResult CCEffectParametersTest::OnTriggerEffect(CCEffectInstanceParameters* effectInstance) {
	std::cout << effectInstance->quantity << "\n";
	std::cout << effectInstance->GetParam("Total Coins") << "\n";
	return EffectResult::Success;
}