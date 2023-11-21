#include "CCEffectInstanceTimed.hpp"
#include "RPC.hpp"
#include <chrono>
#include <memory>
#include <iostream>
#include <string>
#include <cstdlib>

int CCEffectInstanceTimed::TimeRemaining() {
	if (timedEffect->paused) {
		auto pausedElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(pauseTime - startTime);
		return static_cast<int>((runTime * 1000) - pausedElapsed.count());
	}
	else {
		auto now = std::chrono::steady_clock::now();
		auto totalElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime) - pausedDuration;
		return static_cast<int>((runTime * 1000) - totalElapsed.count());
	}
}

void CCEffectInstanceTimed::Pause() {
	if (!timedEffect->paused) {
		timedEffect->paused = true;
		timedEffect->OnPause();

		pauseTime = std::chrono::steady_clock::now();
		RPC::TimedPause(*this);
	}
}

void CCEffectInstanceTimed::Resume() {
	if (timedEffect->paused) {
		timedEffect->paused = false;
		timedEffect->OnResume();

		auto currentPauseDuration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - pauseTime);
		pausedDuration += currentPauseDuration;
		RPC::TimedResume(*this);
	} 
}

void CCEffectInstanceTimed::Reset() {
	startTime = std::chrono::steady_clock::now();
}

void CCEffectInstanceTimed::Stop(bool force) {
	RPC::TimedEnd(*this);
	timedEffect->OnStopEffect(force);
}