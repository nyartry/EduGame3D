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
	constexpr float ProbeButtonX = 32.0f;
	constexpr float ProbeButtonY = 32.0f;
	constexpr float ProbeButtonWidth = 270.0f;
	constexpr float ProbeButtonHeight = 70.0f;
	constexpr float SampleButtonY = 32.0f;
	constexpr float SampleButtonWidth = 145.0f;
	constexpr float SampleButtonHeight = 70.0f;
	constexpr float SampleButtonGap = 12.0f;
	constexpr float SampleButtonX0 = ProbeButtonX + ProbeButtonWidth + 28.0f;
	constexpr float SampleButtonX1 = SampleButtonX0 + SampleButtonWidth + SampleButtonGap;
	constexpr float SampleButtonX2 = SampleButtonX1 + SampleButtonWidth + SampleButtonGap;
	constexpr float SampleButtonX3 = SampleButtonX2 + SampleButtonWidth + SampleButtonGap;

	constexpr std::string_view TitleMarkup = R"(
<rml>
<head>
	<style>
		body { width: 100vw; height: 100vh; margin: 0; padding: 0; }
		#probe {
			position: absolute; left: 32px; top: 32px; width: 270px; height: 70px;
			background-color: rgb(34, 96, 55); border-width: 4px;
			border-color: rgb(121, 255, 166); padding: 0;
		}
		#probe:hover { background-color: rgb(45, 132, 74); border-color: rgb(255, 220, 99); }
		.sample-button {
			position: absolute; top: 32px; width: 145px; height: 70px;
			padding: 0; border-width: 3px;
		}
		#sample-a { left: 330px; background-color: rgb(22, 65, 105); border-color: rgb(74, 190, 255); }
		#sample-a:hover { background-color: rgb(32, 92, 148); border-color: rgb(154, 225, 255); }
		#sample-b { left: 487px; background-color: rgb(77, 47, 15); border-color: rgb(255, 183, 72); }
		#sample-b:hover { background-color: rgb(112, 69, 24); border-color: rgb(255, 222, 128); }
		#sample-c { left: 644px; background-color: rgb(75, 28, 42); border-color: rgb(255, 102, 139); }
		#sample-c:hover { background-color: rgb(117, 39, 62); border-color: rgb(255, 174, 194); }
		#sample-d { left: 801px; background-color: rgb(27, 36, 56); border-color: rgb(184, 204, 230); }
		#sample-d:hover { background-color: rgb(47, 59, 86); border-color: rgb(255, 255, 255); }
		#menu { position: absolute; left: 430px; top: 418px; width: 420px; }
		#menu button {
			display: block; width: 420px; height: 58px; margin-bottom: 18px;
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

	bool IsPointerInside(const Input& input, float x, float y, float width, float height)
	{
		return input.WasLeftMousePressed() &&
			input.IsMouseInsideClient() &&
			static_cast<float>(input.GetMouseX()) >= x &&
			static_cast<float>(input.GetMouseX()) <= x + width &&
			static_cast<float>(input.GetMouseY()) >= y &&
			static_cast<float>(input.GetMouseY()) <= y + height;
	}
}

void TitleScene::Load(const SceneLoadContext& context)
{
	m_width = context.width;
	m_height = context.height;
	m_audio = context.audio;
	m_batch.Initialize(*context.renderDevice, 4096);
	if (context.ui != nullptr)
	{
		m_uiDocument = context.ui->CreateDocument("title", m_width, m_height, TitleMarkup);
	}
	RebuildBatch();
}

void TitleScene::Unload()
{
	m_uiDocument.reset();
}

void TitleScene::Update(float deltaTime, const Input& input)
{
	m_elapsedTime += deltaTime;
	m_probeButtonFlashTime = std::max(0.0f, m_probeButtonFlashTime - deltaTime);
	if (m_audio != nullptr)
	{
		m_audio->PlayBgm(GameContent::TitleBgm);
	}

	if (m_uiDocument != nullptr && input.IsMouseInsideClient())
	{
		m_uiDocument->ProcessPointerMove(input.GetMouseX(), input.GetMouseY());
		if (input.WasLeftMousePressed())
		{
			m_uiDocument->ProcessPointerButtonDown(0);
		}
		if (input.WasLeftMouseReleased())
		{
			m_uiDocument->ProcessPointerButtonUp(0);
		}
	}

	const float width = static_cast<float>(m_width);
	const float height = static_cast<float>(m_height);
	const float startButtonLeft = width * 0.5f - 210.0f;
	const float startButtonTop = height * 0.58f;
	const bool clickedProbeButton = IsPointerInside(input, ProbeButtonX, ProbeButtonY, ProbeButtonWidth, ProbeButtonHeight);
	const bool clickedStartButton = IsPointerInside(input, startButtonLeft, startButtonTop, 420.0f, 58.0f);

	if (clickedProbeButton)
	{
		if (m_audio != nullptr)
		{
			m_audio->PlaySe(GameContent::ButtonSe);
		}
		++m_probeButtonClickCount;
		m_probeButtonFlashTime = 0.35f;
		if (m_uiDocument != nullptr)
		{
			m_uiDocument->SetElementStyle("probe", "background-color", "rgb(128, 82, 24)");
			m_uiDocument->SetElementStyle("probe", "border-color", "rgb(255, 220, 99)");
		}
	}
	else if (m_probeButtonFlashTime <= 0.0f && m_uiDocument != nullptr)
	{
		m_uiDocument->SetElementStyle("probe", "background-color", "rgb(34, 96, 55)");
		m_uiDocument->SetElementStyle("probe", "border-color", "rgb(121, 255, 166)");
	}

	int clickedSampleButton = 0;
	const float sampleXs[] = { SampleButtonX0, SampleButtonX1, SampleButtonX2, SampleButtonX3 };
	for (int index = 0; index < 4; ++index)
	{
		if (IsPointerInside(input, sampleXs[index], SampleButtonY, SampleButtonWidth, SampleButtonHeight))
		{
			clickedSampleButton = index + 1;
			break;
		}
	}

	if (clickedSampleButton != 0 && m_uiDocument != nullptr)
	{
		if (m_audio != nullptr)
		{
			m_audio->PlaySe(GameContent::ButtonSe);
		}
		m_selectedSampleButton = clickedSampleButton;
		++m_sampleButtonClickCount;

		const std::string_view ids[] = { "sample-a", "sample-b", "sample-c", "sample-d" };
		const std::string_view normalBackgrounds[] =
		{
			"rgb(22, 65, 105)", "rgb(77, 47, 15)", "rgb(75, 28, 42)", "rgb(27, 36, 56)"
		};
		const std::string_view normalBorders[] =
		{
			"rgb(74, 190, 255)", "rgb(255, 183, 72)", "rgb(255, 102, 139)", "rgb(184, 204, 230)"
		};
		for (int index = 0; index < 4; ++index)
		{
			const bool selected = index + 1 == clickedSampleButton;
			m_uiDocument->SetElementStyle(
				ids[index],
				"background-color",
				selected ? "rgb(14, 122, 102)" : normalBackgrounds[index]);
			m_uiDocument->SetElementStyle(
				ids[index],
				"border-color",
				selected ? "rgb(121, 255, 229)" : normalBorders[index]);
		}
	}

	if (GameActions::WasPressed(input, GameAction::Confirm) || clickedStartButton)
	{
		if (!m_startRequested && m_audio != nullptr)
		{
			m_audio->PlaySe(GameContent::ButtonSe);
		}
		m_startRequested = true;
	}
	RebuildBatch();
}

void TitleScene::Render(IRenderer& renderer) const
{
	m_batch.Render(renderer);
}

XMMATRIX TitleScene::GetViewProjectionMatrix() const
{
	return XMMatrixIdentity();
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
	const float pulse = (std::sin(m_elapsedTime * 3.2f) + 1.0f) * 0.5f;
	const float titleSize = std::clamp(width / 118.0f, 4.0f, 8.0f);
	const float subtitleSize = std::clamp(width / 260.0f, 2.0f, 4.0f);
	const float promptSize = std::clamp(width / 260.0f, 2.6f, 4.4f);
	const float centerX = width * 0.5f;
	const float centerY = height * 0.5f;

	m_batch.Clear();
	m_batch.DrawRectangle(0.0f, 0.0f, width, height, BackgroundColor);
	m_batch.DrawRectangle(0.0f, height * 0.22f, width, 2.0f, AccentDarkColor);
	m_batch.DrawRectangle(0.0f, height * 0.78f, width, 2.0f, AccentDarkColor);
	const float panelWidth = std::min(width - 80.0f, 760.0f);
	const float panelHeight = std::min(height * 0.42f, 310.0f);
	const float panelX = centerX - panelWidth * 0.5f;
	const float panelY = centerY - panelHeight * 0.5f;
	m_batch.DrawRectangle(panelX, panelY, panelWidth, panelHeight, { 0.025f, 0.04f, 0.065f, 0.72f });
	m_batch.DrawRectangle(panelX, panelY, 5.0f, panelHeight, AccentColor);
	m_batch.DrawRectangle(panelX + panelWidth - 5.0f, panelY, 5.0f, panelHeight, AccentColor);
	const float markerWidth = 88.0f + 18.0f * pulse;
	m_batch.DrawRectangle(centerX - markerWidth * 0.5f, panelY + 34.0f, markerWidth, 5.0f, PromptColor);
	m_batch.DrawRectangle(centerX - markerWidth * 0.5f, panelY + panelHeight - 39.0f, markerWidth, 5.0f, PromptColor);

	DrawCenteredText("TITLE", centerY - 70.0f, titleSize, TitleColor);
	DrawCenteredText("OPEN CAMPUS GAME", centerY + 8.0f, subtitleSize, SubtleTextColor);
	if (m_uiDocument != nullptr)
	{
		m_uiDocument->RenderTo(m_batch);
	}
	if (m_uiDocument == nullptr || !m_uiDocument->IsLoaded())
	{
		m_batch.DrawText("UI LOAD FAILED", 32.0f, 96.0f, 2.0f, PromptColor);
		if (m_uiDocument != nullptr)
		{
			m_batch.DrawText(m_uiDocument->GetLastError().substr(0, 46), 32.0f, 122.0f, 1.5f, SubtleTextColor);
		}
	}
	else if (m_uiDocument->GetLastRenderedTriangleCount() == 0)
	{
		m_batch.DrawText("UI ZERO GEOMETRY", 32.0f, 96.0f, 2.0f, PromptColor);
	}

	m_batch.DrawText("UI BUTTON", ProbeButtonX + 24.0f, ProbeButtonY + 18.0f, 2.1f, UiLabelColor);
	const std::string probeText = m_probeButtonClickCount > 0 ? "CLICKED " + std::to_string(m_probeButtonClickCount) : "CLICK ME";
	m_batch.DrawText(probeText, ProbeButtonX + 24.0f, ProbeButtonY + 44.0f, 1.6f, PromptColor);
	m_batch.DrawText("BLUE", SampleButtonX0 + 18.0f, SampleButtonY + 20.0f, 1.55f, UiLabelColor);
	m_batch.DrawText("FLAT", SampleButtonX0 + 18.0f, SampleButtonY + 46.0f, 1.25f, SubtleTextColor);
	m_batch.DrawText("GOLD", SampleButtonX1 + 18.0f, SampleButtonY + 20.0f, 1.55f, UiLabelColor);
	m_batch.DrawText("ALERT", SampleButtonX1 + 18.0f, SampleButtonY + 46.0f, 1.25f, PromptColor);
	m_batch.DrawText("RED", SampleButtonX2 + 18.0f, SampleButtonY + 20.0f, 1.55f, UiLabelColor);
	m_batch.DrawText("DANGER", SampleButtonX2 + 18.0f, SampleButtonY + 46.0f, 1.25f, SubtleTextColor);
	m_batch.DrawText("STEEL", SampleButtonX3 + 14.0f, SampleButtonY + 20.0f, 1.35f, UiLabelColor);
	m_batch.DrawText("OUTLINE", SampleButtonX3 + 14.0f, SampleButtonY + 46.0f, 1.15f, SubtleTextColor);
	if (m_selectedSampleButton != 0)
	{
		const std::string selectedText = "SAMPLE " + std::to_string(m_selectedSampleButton) + " CLICKED " + std::to_string(m_sampleButtonClickCount);
		m_batch.DrawText(selectedText, SampleButtonX0, SampleButtonY + 88.0f, 1.7f, PromptColor);
	}
	DrawCenteredText("START GAME", height * 0.58f + 18.0f, 3.0f, UiLabelColor);
	DrawCenteredText("EXIT", height * 0.58f + 94.0f, 3.0f, SubtleTextColor);
	DrawCenteredText("PRESS ENTER", centerY + 245.0f + pulse * 6.0f, promptSize, PromptColor);
	m_batch.Upload();
}

void TitleScene::DrawCenteredText(std::string_view text, float centerY, float pixelSize, const XMFLOAT4& color)
{
	const XMFLOAT2 textSize = m_batch.MeasureText(text, pixelSize);
	const float x = (static_cast<float>(m_width) - textSize.x) * 0.5f;
	const float y = centerY - textSize.y * 0.5f;
	m_batch.DrawText(text, x, y, pixelSize, color);
}
