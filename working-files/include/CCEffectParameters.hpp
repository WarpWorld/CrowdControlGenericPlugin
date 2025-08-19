#pragma once
#include "CCEffectBase.hpp"
#include "ParameterEntry.hpp"
#include "DLLConfig.hpp"
#include <unordered_map>

class CCEffectInstanceParameters;

class CC_API CCEffectParameters : public CCEffectBase {
public:
	CCEffectParameters();

	std::unordered_map<std::string, ParameterEntry> parameterEntries;

	virtual EffectResult OnTriggerEffect(CCEffectInstanceParameters* effectInstance) = 0;
	virtual void SetupParams() = 0;

	nlohmann::json JSONManifest() override {
		nlohmann::json manifest = CCEffectBase::JSONManifest();

		for (const auto& pair : parameterEntries) {
			manifest["parameters"][pair.first] = pair.second.JSONManifest();
		}

		return manifest;
	}

	void AddOptionsParameter(char * name, char ** options) {
		std::string nameStr(name);
		ParameterEntry entry(nameStr, id, ParameterEntry::Kind::Item);

		if (options != nullptr) {
			for (int i = 0; options[i] != nullptr; ++i) {
				entry.AddOption(options[i]);
			}
		}

		parameterEntries[entry.id] = entry;
	}

	void AddMinMaxParameter(char * name, int min, int max) {
		std::string nameStr(name);
		ParameterEntry entry(nameStr, id, ParameterEntry::Kind::Quantity);

		entry.SetMinMax(min, max);
		parameterEntries[entry.id] = entry;
	}
};
