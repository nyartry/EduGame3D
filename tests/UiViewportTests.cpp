#include "TestSupport.h"
#include "Framework/UI/RmlUi/RmlUiService.h"

#include <cmath>
#include <cstdint>
#include <utility>

namespace
{
	void Require(bool condition, const char* message)
	{
		if (!condition) throw std::runtime_error(message);
	}

	// Padding and borders deliberately differ from the content dimensions so a
	// content-box result cannot accidentally pass the sprite-label contract.
	constexpr std::string_view Markup = R"(
<rml><head><style>
	body { width: 100vw; height: 100vh; margin: 0; padding: 0; }
	#target {
		position: absolute; left: 60vw; top: 65vh; width: 25vw; height: 10vh;
		box-sizing: content-box; margin: 0; padding: 3px; border-width: 4px;
		background-color: rgb(22, 65, 105); border-color: rgb(74, 190, 255);
	}
</style></head><body><button id="target"></button></body></rml>
)";

	struct Fixture
	{
		// The document must be destroyed before the service shuts RmlUi down.
		RmlUiService service;
		std::unique_ptr<IUiDocument> document{ service.CreateDocument("viewport-test", 1280, 720, Markup) };
		Fixture()
		{
			Require(document != nullptr && document->IsLoaded(), "UI document is loaded without a renderer or GPU");
			document->WatchClick("target");
		}
	};

	UiElementBounds Bounds(IUiDocument& document)
	{
		const auto bounds = document.GetElementBounds("target");
		Require(bounds.has_value(), "Existing element has a border box");
		return *bounds;
	}

	void RequireBounds(const UiElementBounds& bounds, float x, float y, float width, float height)
	{
		constexpr float tolerance = 0.6f;
		Require(std::abs(bounds.x - x) < tolerance && std::abs(bounds.y - y) < tolerance &&
			std::abs(bounds.width - width) < tolerance && std::abs(bounds.height - height) < tolerance,
			"Element reports its current border box in client pixels");
	}

	void Click(IUiDocument& document, int x, int y)
	{
		document.ProcessPointerMove(x, y);
		document.ProcessPointerButtonDown(0);
		document.ProcessPointerButtonUp(0);
	}

	void ClickCenter(IUiDocument& document, const UiElementBounds& bounds)
	{
		Click(document, static_cast<int>(bounds.x + bounds.width * 0.5f),
			static_cast<int>(bounds.y + bounds.height * 0.5f));
	}

	void InitialLayoutExposesBorderBounds()
	{
		Fixture fixture;
		const auto bounds = Bounds(*fixture.document);
		RequireBounds(bounds, 768.0f, 468.0f, 334.0f, 86.0f);
		Require(!fixture.document->GetElementBounds("missing"), "Unknown element has no fabricated bounds");
		ClickCenter(*fixture.document, bounds);
		Require(fixture.document->ConsumeClick("target"), "Initial layout accepts direct client pixel input");
	}

	void ResizeUpdatesFirstClickBeforeRendering()
	{
		Fixture fixture;
		const auto original = Bounds(*fixture.document);
		fixture.document->Resize(640, 360);
		const auto resized = Bounds(*fixture.document);
		RequireBounds(resized, 384.0f, 234.0f, 174.0f, 50.0f);
		// No RenderTo or extra layout tick intervenes between Resize and input.
		ClickCenter(*fixture.document, resized);
		Require(fixture.document->ConsumeClick("target"), "First click after resizing uses the new layout");
		Require(!fixture.document->ConsumeClick("target"), "A click is consumed only once");
		ClickCenter(*fixture.document, original);
		Require(!fixture.document->ConsumeClick("target"), "Old client coordinates no longer hit the moved button");
	}

	void ZeroDimensionsPreserveLayoutAndInput()
	{
		Fixture fixture;
		fixture.document->Resize(800, 600);
		const auto valid = Bounds(*fixture.document);
		for (const auto [width, height] : { std::pair{ 0u, 600u }, std::pair{ 800u, 0u }, std::pair{ 0u, 0u } })
		{
			fixture.document->Resize(width, height);
			RequireBounds(Bounds(*fixture.document), valid.x, valid.y, valid.width, valid.height);
			ClickCenter(*fixture.document, valid);
			Require(fixture.document->ConsumeClick("target"), "Zero dimensions retain the last valid click target");
		}
	}

	void RepeatedResizeAndRestoreKeepListeners()
	{
		Fixture fixture;
		for (const auto [width, height] : { std::pair{ 1600u, 900u }, std::pair{ 480u, 800u },
			std::pair{ 1280u, 720u }, std::pair{ 1280u, 720u } })
		{
			fixture.document->Resize(width, height);
			const auto bounds = Bounds(*fixture.document);
			RequireBounds(bounds, width * 0.60f, height * 0.65f, width * 0.25f + 14.0f, height * 0.10f + 14.0f);
			// A point inside the border is part of the same reported clickable box.
			Click(*fixture.document, static_cast<int>(bounds.x) + 2, static_cast<int>(bounds.y) + 2);
			Require(fixture.document->ConsumeClick("target"), "Resizing preserves click listeners and border hit testing");
		}
	}
}

#define UI_VIEWPORT_TEST_CASES(TEST) \
	TEST(InitialLayoutExposesBorderBounds, "UI initial layout exposes border bounds in client pixels", Cpu) \
	TEST(ResizeUpdatesFirstClickBeforeRendering, "UI resizing updates the first click before rendering", Cpu) \
	TEST(ZeroDimensionsPreserveLayoutAndInput, "UI zero dimensions preserve layout and input", Cpu) \
	TEST(RepeatedResizeAndRestoreKeepListeners, "UI repeated resizing and restore preserve click listeners", Cpu)

GAME_TEST_SUITE(UiViewportTests, UI_VIEWPORT_TEST_CASES)
