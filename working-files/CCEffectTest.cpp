#include "include/CCEffectTest.hpp"
#include "include/StreamBuf.hpp"
#include "pch.h"

CCEffectTest::CCEffectTest() {

}

EffectResult CCEffectTest::OnTriggerEffect(CCEffectInstance* effectInstance) {
	Streambuf::Important("TRIGGERED");
	return EffectResult::Success;
}
