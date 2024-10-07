#pragma once
#include "include/CCEffectTimed.hpp"

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

void CCEffectTimed::SetupTimed(char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray, float d) {
	Setup(name, desc, price, retries, retryDelay, pendingDelay, sellable, visible, nonPoolable, morality, orderliness, categoriesArray);
	duration = d / 1000.0f;
}
