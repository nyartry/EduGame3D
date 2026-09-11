#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

class SpriteBatch;

struct UiElementBounds
{
	float x{};
	float y{};
	float width{};
	float height{};
};

class IUiDocument
{
public:
	virtual ~IUiDocument() = default;

	// Layout and pointer input use the same client pixel coordinates. A zero
	// dimension (for example, while minimized) preserves the last valid layout.
	virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;
	// The border box from the latest layout lets sprite labels follow controls.
	virtual std::optional<UiElementBounds> GetElementBounds(std::string_view elementId) const = 0;
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
