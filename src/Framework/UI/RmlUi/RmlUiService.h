#pragma once

#include "Framework/UI/IUiService.h"

#include <cstdint>

// RmlUi is an adapter behind IUiService. Its source remains part of the engine
// project, while game scenes only depend on the vendor-neutral UI contract.
class RmlUiService final : public IUiService
{
public:
	~RmlUiService() override;

	bool Initialize();
	void Shutdown();

	std::unique_ptr<IUiDocument> CreateDocument(
		std::string_view name,
		std::uint32_t width,
		std::uint32_t height,
		std::string_view markup) override;

private:
	std::uint32_t m_nextContextId{};
	bool m_initialized{};
};
