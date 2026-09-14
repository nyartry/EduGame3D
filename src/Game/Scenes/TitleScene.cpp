#include "Game/Scenes/TitleScene.h"

#include "Framework/Audio/IAudioService.h"
#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Framework/Rendering/Core/IRenderer.h"
#include "Game/Content/GameContent.h"
#include "Game/Input/GameActions.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>

using namespace DirectX;

namespace
{
	constexpr const char* NextSceneName = "Game";
	constexpr XMFLOAT4 BackgroundColor{ 0.015f, 0.025f, 0.045f, 0.88f };
	constexpr XMFLOAT4 AccentColor{ 0.26f, 0.78f, 1.0f, 0.88f };
	constexpr XMFLOAT4 AccentDarkColor{ 0.10f, 0.22f, 0.32f, 0.92f };
	constexpr XMFLOAT4 TitleColor{ 0.92f, 0.98f, 1.0f, 1.0f };
	constexpr XMFLOAT4 PromptColor{ 1.0f, 0.78f, 0.32f, 0.96f };
	constexpr XMFLOAT4 SubtleTextColor{ 0.62f, 0.74f, 0.84f, 0.92f };
	constexpr XMFLOAT4 UiLabelColor{ 0.95f, 0.99f, 1.0f, 1.0f };
	constexpr std::string_view SampleButtonIds[] = { "sample-a", "sample-b", "sample-c", "sample-d" };

	// Viewport units keep the sample controls visible as the client area changes.
	// Sprite labels below read these controls' actual border boxes from the UI.
	constexpr std::string_view TitleMarkup = R"(
<rml>
<head>
	<style>
		body { width: 100vw; height: 100vh; margin: 0; padding: 0; }
		button { box-sizing: border-box; }
		#probe {
			position: absolute; left: 2.5vw; top: 4.444444vh; width: 21.71875vw; height: 10.833333vh;
			background-color: rgb(34, 96, 55); border-width: 4px;
			border-color: rgb(121, 255, 166); padding: 0;
		}
		#probe:hover { background-color: rgb(45, 132, 74); border-color: rgb(255, 220, 99); }
		#probe.flashed { background-color: rgb(128, 82, 24); border-color: rgb(255, 220, 99); }
		.sample-button {
			position: absolute; top: 4.444444vh; width: 11.796875vw; height: 10.555556vh;
			padding: 0; border-width: 3px;
		}
		#sample-a { left: 25.78125vw; background-color: rgb(22, 65, 105); border-color: rgb(74, 190, 255); }
		#sample-a:hover { background-color: rgb(32, 92, 148); border-color: rgb(154, 225, 255); }
		#sample-b { left: 38.046875vw; background-color: rgb(77, 47, 15); border-color: rgb(255, 183, 72); }
		#sample-b:hover { background-color: rgb(112, 69, 24); border-color: rgb(255, 222, 128); }
		#sample-c { left: 50.3125vw; background-color: rgb(75, 28, 42); border-color: rgb(255, 102, 139); }
		#sample-c:hover { background-color: rgb(117, 39, 62); border-color: rgb(255, 174, 194); }
		#sample-d { left: 62.578125vw; background-color: rgb(27, 36, 56); border-color: rgb(184, 204, 230); }
		#sample-d:hover { background-color: rgb(47, 59, 86); border-color: rgb(255, 255, 255); }
		#sample-a.selected { background-color: rgb(14, 122, 102); border-color: rgb(121, 255, 229); }
		#sample-b.selected { background-color: rgb(14, 122, 102); border-color: rgb(121, 255, 229); }
		#sample-c.selected { background-color: rgb(14, 122, 102); border-color: rgb(121, 255, 229); }
		#sample-d.selected { background-color: rgb(14, 122, 102); border-color: rgb(121, 255, 229); }
		#menu { position: absolute; left: 33.4375vw; top: 58vh; width: 33.125vw; }
		#menu button {
			display: block; width: 100%; height: 8.611111vh; margin-bottom: 2.5vh;
			background-color: rgb(13, 49, 71); border-width: 2px;
			border-color: rgb(82, 215, 255); padding: 0;
		}
		#menu button:hover { background-color: rgb(20, 88, 120); border-color: rgb(255, 213, 111); }
		#exit { background-color: rgb(21, 35, 52); border-color: rgb(109, 129, 146); }
	</style>
</head>
<body>
	<button id="probe"></button>
	<button class="sample-button" id="sample-a"></button>
	<button class="sample-button" id="sample-b"></button>
	<button class="sample-button" id="sample-c"></button>
	<button class="sample-button" id="sample-d"></button>
	<div id="menu"><button id="start"></button><button id="exit"></button></div>
</body>
</rml>
)";
}

TitleScene::TitleScene(
	IRenderDevice& renderDevice,
	IAudioService& audio,
	IUiService& ui,
	std::uint32_t width,
	std::uint32_t height)
	: m_renderDevice(renderDevice)
	, m_audio(audio)
	, m_ui(ui)
	, m_width(width)
	, m_height(height)
{
}

void TitleScene::Activate()
{
	m_batch.Initialize(m_renderDevice, 4096);
	m_uiDocument = m_ui.CreateDocument("title", m_width, m_height, TitleMarkup);
	if (m_uiDocument != nullptr)
	{
		m_uiDocument->WatchClick("probe");
		m_uiDocument->WatchClick("start");
		for (const std::string_view id : SampleButtonIds)
		{
			m_uiDocument->WatchClick(id);
		}
	}
	RebuildBatch();
	m_active = true;
}

void TitleScene::Unload()
{
	m_active = false;
	m_uiDocument.reset();
}

void TitleScene::OnResize(std::uint32_t width, std::uint32_t height)
{
	if (width == 0 || height == 0)
	{
		return;
	}
	m_width = width;
	m_height = height;
	if (!m_active)
	{
		return;
	}
	if (m_uiDocument != nullptr)
	{
		m_uiDocument->Resize(width, height);
	}
	RebuildBatch();
}

void TitleScene::UpdateFrame(float deltaTime, const Input& input)
{
	m_elapsedTime += deltaTime;
	m_probeButtonFlashTime = std::max(0.0f, m_probeButtonFlashTime - deltaTime);
	m_audio.PlayBgm(GameContent::TitleBgm);

	if (m_uiDocument != nullptr)
	{
		if (input.IsMouseInsideClient())
		{
			m_uiDocument->ProcessPointerMove(input.GetMouseX(), input.GetMouseY());
			if (input.WasLeftMousePressed())
			{
				m_uiDocument->ProcessPointerButtonDown(0);
			}
		}
		else
		{
			m_uiDocument->ProcessPointerLeave();
		}
		if (input.WasLeftMouseReleased())
		{
			m_uiDocument->ProcessPointerButtonUp(0);
		}
	}

	const bool clickedProbeButton = m_uiDocument != nullptr && m_uiDocument->ConsumeClick("probe");
	const bool clickedStartButton = m_uiDocument != nullptr && m_uiDocument->ConsumeClick("start");

	if (clickedProbeButton)
	{
		m_audio.PlaySe(GameContent::ButtonSe);
		++m_probeButtonClickCount;
		m_probeButtonFlashTime = 0.35f;
	}
	if (m_uiDocument != nullptr)
	{
		m_uiDocument->SetElementClass("probe", "flashed", m_probeButtonFlashTime > 0.0f);
	}

	int clickedSampleButton = 0;
	if (m_uiDocument != nullptr)
	{
		for (int index = 0; index < 4; ++index)
		{
			if (m_uiDocument->ConsumeClick(SampleButtonIds[index]))
			{
				clickedSampleButton = index + 1;
				break;
			}
		}
	}

	if (clickedSampleButton != 0 && m_uiDocument != nullptr)
	{
		m_audio.PlaySe(GameContent::ButtonSe);
		m_selectedSampleButton = clickedSampleButton;
		++m_sampleButtonClickCount;

		for (int index = 0; index < 4; ++index)
		{
			m_uiDocument->SetElementClass(SampleButtonIds[index], "selected", index + 1 == clickedSampleButton);
		}
	}

	if (GameActions::WasPressed(input, GameAction::Confirm) || clickedStartButton)
	{
		if (!m_startRequested)
		{
			m_audio.PlaySe(GameContent::ButtonSe);
		}
		m_startRequested = true;
		m_loadFailed = false;
	}
	RebuildBatch();
}

void TitleScene::RenderOverlay(IRenderer& renderer) const
{
	m_batch.Render(renderer);
}

RenderView TitleScene::GetRenderView() const
{
	return {};
}

std::string TitleScene::GetRequestedSceneName() const
{
	return m_startRequested ? NextSceneName : std::string{};
}

bool TitleScene::ShouldLoadRequestedSceneAsync() const
{
	return m_startRequested;
}

void TitleScene::RebuildBatch()
{
	const float width = static_cast<float>(m_width);
	const float height = static_cast<float>(m_height);
	const float uiScale = std::min(width / 1280.0f, height / 720.0f);
	const float pulse = (std::sin(m_elapsedTime * 3.2f) + 1.0f) * 0.5f;
	const float titleSize = 8.0f * uiScale;
	const float subtitleSize = 4.0f * uiScale;
	const float promptSize = 4.4f * uiScale;
	const float centerX = width * 0.5f;
	const float centerY = height * 0.5f;

	m_batch.Clear();
	m_batch.DrawRectangle(0.0f, 0.0f, width, height, BackgroundColor);
	m_batch.DrawRectangle(0.0f, height * 0.22f, width, 2.0f * uiScale, AccentDarkColor);
	m_batch.DrawRectangle(0.0f, height * 0.78f, width, 2.0f * uiScale, AccentDarkColor);
	const float panelWidth = std::min(width - 80.0f * uiScale, 760.0f * uiScale);
	const float panelHeight = std::min(height * 0.42f, 310.0f * uiScale);
	const float panelX = centerX - panelWidth * 0.5f;
	const float panelY = centerY - panelHeight * 0.5f;
	m_batch.DrawRectangle(panelX, panelY, panelWidth, panelHeight, { 0.025f, 0.04f, 0.065f, 0.72f });
	m_batch.DrawRectangle(panelX, panelY, 5.0f * uiScale, panelHeight, AccentColor);
	m_batch.DrawRectangle(panelX + panelWidth - 5.0f * uiScale, panelY, 5.0f * uiScale, panelHeight, AccentColor);
	const float markerWidth = (88.0f + 18.0f * pulse) * uiScale;
	m_batch.DrawRectangle(centerX - markerWidth * 0.5f, panelY + 34.0f * uiScale, markerWidth, 5.0f * uiScale, PromptColor);
	m_batch.DrawRectangle(centerX - markerWidth * 0.5f, panelY + panelHeight - 39.0f * uiScale, markerWidth, 5.0f * uiScale, PromptColor);

	DrawCenteredText("EDUGAME3D", centerY - 70.0f * uiScale, titleSize, TitleColor);
	DrawCenteredText("3D GAME PROGRAMMING", centerY + 8.0f * uiScale, subtitleSize, SubtleTextColor);
	if (m_uiDocument != nullptr)
	{
		m_uiDocument->RenderTo(m_batch);
	}
	if (m_uiDocument == nullptr || !m_uiDocument->IsLoaded())
	{
		m_batch.DrawText("UI LOAD FAILED", width * 0.025f, height * 0.17f, 2.0f * uiScale, PromptColor);
		if (m_uiDocument != nullptr)
		{
			m_batch.DrawText(m_uiDocument->GetLastError().substr(0, 46), width * 0.025f, height * 0.20f, 1.5f * uiScale, SubtleTextColor);
		}
	}
	else if (m_uiDocument->GetLastRenderedTriangleCount() == 0)
	{
		m_batch.DrawText("UI ZERO GEOMETRY", width * 0.025f, height * 0.17f, 2.0f * uiScale, PromptColor);
	}

	DrawButtonText("probe", "UI BUTTON", 0.33f, 2.1f * uiScale, UiLabelColor);
	const std::string probeText = m_probeButtonClickCount > 0 ? "CLICKED " + std::to_string(m_probeButtonClickCount) : "CLICK ME";
	DrawButtonText("probe", probeText, 0.66f, 1.6f * uiScale, PromptColor);
	DrawButtonText("sample-a", "BLUE", 0.34f, 1.55f * uiScale, UiLabelColor);
	DrawButtonText("sample-a", "FLAT", 0.68f, 1.25f * uiScale, SubtleTextColor);
	DrawButtonText("sample-b", "GOLD", 0.34f, 1.55f * uiScale, UiLabelColor);
	DrawButtonText("sample-b", "ALERT", 0.68f, 1.25f * uiScale, PromptColor);
	DrawButtonText("sample-c", "RED", 0.34f, 1.55f * uiScale, UiLabelColor);
	DrawButtonText("sample-c", "DANGER", 0.68f, 1.25f * uiScale, SubtleTextColor);
	DrawButtonText("sample-d", "STEEL", 0.34f, 1.35f * uiScale, UiLabelColor);
	DrawButtonText("sample-d", "OUTLINE", 0.68f, 1.15f * uiScale, SubtleTextColor);
	if (m_selectedSampleButton != 0 && m_uiDocument != nullptr)
	{
		const std::string selectedText = "SAMPLE " + std::to_string(m_selectedSampleButton) + " CLICKED " + std::to_string(m_sampleButtonClickCount);
		if (const auto bounds = m_uiDocument->GetElementBounds("sample-a"))
		{
			m_batch.DrawText(selectedText, std::round(bounds->x), std::round(bounds->y + bounds->height + 12.0f * uiScale),
				std::max(1.0f, 1.7f * uiScale), PromptColor);
		}
	}
	DrawButtonText("start", "START GAME", 0.5f, 3.0f * uiScale, UiLabelColor);
	DrawButtonText("exit", "EXIT", 0.5f, 3.0f * uiScale, SubtleTextColor);
	DrawCenteredText("PRESS ENTER", height * 0.84f + pulse * 6.0f * uiScale, promptSize, PromptColor);
	if (m_loadFailed)
	{
		DrawCenteredText("LOAD FAILED - PRESS ENTER TO RETRY", height - 32.0f * uiScale, 2.0f * uiScale, PromptColor);
	}
	m_batch.Upload();
}

void TitleScene::DrawCenteredText(std::string_view text, float centerY, float pixelSize, const XMFLOAT4& color)
{
	const XMFLOAT2 textSize = m_batch.MeasureText(text, pixelSize);
	const float x = (static_cast<float>(m_width) - textSize.x) * 0.5f;
	const float y = centerY - textSize.y * 0.5f;
	m_batch.DrawText(text, x, y, pixelSize, color);
}

void TitleScene::DrawButtonText(std::string_view elementId, std::string_view text, float centerYRatio,
	float pixelSize, const XMFLOAT4& color)
{
	if (m_uiDocument == nullptr)
	{
		return;
	}
	const auto bounds = m_uiDocument->GetElementBounds(elementId);
	if (!bounds || bounds->width <= 0.0f || bounds->height <= 0.0f)
	{
		return;
	}
	const XMFLOAT2 unitSize = m_batch.MeasureText(text, 1.0f);
	if (unitSize.x <= 0.0f || unitSize.y <= 0.0f)
	{
		return;
	}
	const float lineHeightRatio = centerYRatio == 0.5f ? 0.5f : 0.22f;
	// Subpixel bitmap cells can disappear when the window is narrow. Keep at
	// least one pixel per cell whenever the control has enough room.
	pixelSize = std::min({ std::max(1.0f, pixelSize), bounds->width * 0.85f / unitSize.x, bounds->height * lineHeightRatio / unitSize.y });
	const XMFLOAT2 textSize = m_batch.MeasureText(text, pixelSize);
	m_batch.DrawText(text, std::round(bounds->x + (bounds->width - textSize.x) * 0.5f),
		std::round(bounds->y + bounds->height * centerYRatio - textSize.y * 0.5f), pixelSize, color);
}
