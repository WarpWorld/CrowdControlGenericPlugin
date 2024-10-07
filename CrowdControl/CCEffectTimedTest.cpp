#include "include/CCEffectTimedTest.hpp"
#include "include/StreamBuf.hpp"

CCEffectTimedTest::CCEffectTimedTest() {

}

EffectResult CCEffectTimedTest::OnTriggerEffect(CCEffectInstanceTimed* effectInstance) {
	Streambuf::Important("TRIGGERED TIMED");
	return EffectResult::Success;
}

bool CCEffectTimedTest::OnStopEffect(bool force) {
	Streambuf::Important("STOP EFFECT");
	return true;
}

void CCEffectTimedTest::OnPause() {
	Streambuf::Important("PAUSE");
}

void CCEffectTimedTest::OnResume() {
	Streambuf::Important("RESUME");
}

void CCEffectTimedTest::OnReset() {
	Streambuf::Important("RESET");
}

void CCEffectTimedTest::OnUpdate() {
	Streambuf::Important("UPDATE");
}

bool CCEffectTimedTest::RunningCondition() {
	return true;
}

