#pragma once
#include "CCEffectTimed.hpp"

CCEffectTimed::CCEffectTimed() {

}

bool CCEffectTimed::ShouldBeRunning() {
	return RunningCondition();
}

bool CCEffectTimed::RunningCondition() {
	return paused;
}

void CCEffectTimed::SetDuration(int durationTime) {
	duration = static_cast<float>(durationTime) / 1000.0f;
}
