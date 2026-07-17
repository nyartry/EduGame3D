#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

class SpriteBatch;

class IUiDocument
{
public:
	virtual ~IUiDocument() = default;

	virtual void ProcessPointerMove(int x, int y) = 0;
	virtual void ProcessPointerLeave() = 0;
	virtual void ProcessPointerButtonDown(int button) = 0;
	virtual void ProcessPointerButtonUp(int button) = 0;
	virtual void WatchClick(std::string_view elementId) = 0;
	virtual bool ConsumeClick(std::string_view elementId) = 0;
	virtual void SetElementClass(std::string_view elementId, std::string_view className, bool enabled) = 0;
	virtual void RenderTo(SpriteBatch& batch) = 0;
	virtual bool IsLoaded() const = 0;
	virtual std::uint32_t GetLastRenderedTriangleCount() const = 0;
	virtual std::string GetLastError() const = 0;
};

class IUiService
{
public:
	virtual ~IUiService() = default;

	virtual std::unique_ptr<IUiDocument> CreateDocument(
		std::string_view name,
		std::uint32_t width,
		std::uint32_t height,
		std::string_view markup) = 0;
};
