#pragma once

#include "SkinnedModelData.h"

#include <string>

class SkinnedModelLoader
{
public:
	bool Load(const std::string& filePath, SkinnedModelData& modelData);
	std::string GetLastError() const;

private:
	std::string m_lastError;
};
