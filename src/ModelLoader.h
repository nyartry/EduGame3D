#pragma once

#include <string>

class ModelLoader
{
public:
	bool Load(const std::string& filePath);
	std::string GetLastError() const;

private:
	std::string m_lastError;
};
