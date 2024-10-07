#include "include/CCEffectTest.hpp"
#include "include/StreamBuf.hpp"

CCEffectTest::CCEffectTest() {

}

EffectResult CCEffectTest::OnTriggerEffect(CCEffectInstance* effectInstance) {
	Streambuf::Important("TRIGGERED");
	return EffectResult::Success;
}
