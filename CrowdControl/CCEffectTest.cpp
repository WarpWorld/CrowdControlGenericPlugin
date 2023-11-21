#include "CCEffectTest.hpp"
#include "StreamBuf.hpp"

CCEffectTest::CCEffectTest() {

}

EffectResult CCEffectTest::OnTriggerEffect(CCEffectInstance* effectInstance) { 
	Streambuf::Important("TRIGGERED");
	return EffectResult::Success;
}
