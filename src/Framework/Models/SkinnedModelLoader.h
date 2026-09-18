#pragma once

#include "Framework/Models/SkinnedModelData.h"

#include <string>

class SkinnedModelLoader
{
public:
	bool Load(const std::string& filePath, SkinnedModelData& modelData);
	bool LoadAnimation(const std::string& filePath, const std::string& animationName, SkinnedModelData& modelData);
	std::string GetLastError() const;

private:
	std::string m_lastError;
};
