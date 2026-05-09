#pragma once

#include "Vertex.h"

#include <string>
#include <vector>

struct ModelData
{
	std::vector<Vertex> vertices;
};

class ModelLoader
{
public:
	bool Load(const std::string& filePath, ModelData& modelData);
	std::string GetLastError() const;

private:
	std::string m_lastError;
};
