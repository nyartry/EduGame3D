#include "Scene/Scenes/TitleScene.h"

#include "Rendering/Core/Dx12Renderer.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FontEngineInterface.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/SystemInterface.h>

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
	constexpr XMFLOAT4 RmlLabelColor{ 0.95f, 0.99f, 1.0f, 1.0f };
	constexpr float ProbeButtonX = 32.0f;
	constexpr float ProbeButtonY = 32.0f;
	constexpr float ProbeButtonWidth = 300.0f;
	constexpr float ProbeButtonHeight = 70.0f;

	class TitleRmlSystemInterface final : public Rml::SystemInterface
	{
	public:
		bool LogMessage(Rml::Log::Type type, const Rml::String& message) override
		{
			(void)type;
			lastMessage = message;
			return true;
		}

		Rml::String lastMessage;
	};

	TitleRmlSystemInterface TitleRmlSystem;

	class TitleRmlFontEngine final : public Rml::FontEngineInterface
	{
	public:
		bool LoadFontFace(const Rml::String& fileName, int faceIndex, bool fallbackFace, Rml::Style::FontWeight weight) override
		{
			(void)fileName;
			(void)faceIndex;
			(void)fallbackFace;
			(void)weight;
			return true;
		}

		bool LoadFontFace(
			const Rml::String& fileName,
			int faceIndex,
			const Rml::String& family,
			Rml::Style::FontStyle style,
			Rml::Style::FontWeight weight,
			bool fallbackFace) override
		{
			(void)fileName;
			(void)faceIndex;
			(void)family;
			(void)style;
			(void)weight;
			(void)fallbackFace;
			return true;
		}

		bool LoadFontFace(
			Rml::Span<const Rml::byte> data,
			int faceIndex,
			const Rml::String& family,
			Rml::Style::FontStyle style,
			Rml::Style::FontWeight weight,
			bool fallbackFace) override
		{
			(void)data;
			(void)faceIndex;
			(void)family;
			(void)style;
			(void)weight;
			(void)fallbackFace;
			return true;
		}

		Rml::FontFaceHandle GetFontFaceHandle(
			const Rml::String& family,
			Rml::Style::FontStyle style,
			Rml::Style::FontWeight weight,
			int size) override
		{
			(void)family;
			(void)style;
			(void)weight;
			m_metrics.size = size;
			m_metrics.ascent = static_cast<float>(size) * 0.8f;
			m_metrics.descent = static_cast<float>(size) * 0.2f;
			m_metrics.line_spacing = static_cast<float>(size) * 1.2f;
			m_metrics.x_height = static_cast<float>(size) * 0.5f;
			m_metrics.underline_position = static_cast<float>(size) * 0.1f;
			m_metrics.underline_thickness = 1.0f;
			m_metrics.has_ellipsis = false;
			return 1;
		}

		Rml::FontEffectsHandle PrepareFontEffects(Rml::FontFaceHandle handle, const Rml::FontEffectList& fontEffects) override
		{
			(void)handle;
			(void)fontEffects;
			return 1;
		}

		const Rml::FontMetrics& GetFontMetrics(Rml::FontFaceHandle handle) override
		{
			(void)handle;
			return m_metrics;
		}

		int GetStringWidth(
			Rml::FontFaceHandle handle,
			Rml::StringView string,
			const Rml::TextShapingContext& textShapingContext,
			Rml::Character priorCharacter = Rml::Character::Null) override
		{
			(void)handle;
			(void)textShapingContext;
			(void)priorCharacter;
			return static_cast<int>(string.size()) * m_metrics.size / 2;
		}

		int GenerateString(
			Rml::RenderManager& renderManager,
			Rml::FontFaceHandle faceHandle,
			Rml::FontEffectsHandle fontEffectsHandle,
			Rml::StringView string,
			Rml::Vector2f position,
			Rml::ColourbPremultiplied colour,
			float opacity,
			const Rml::TextShapingContext& textShapingContext,
			Rml::TexturedMeshList& meshList) override
		{
			(void)renderManager;
			(void)faceHandle;
			(void)fontEffectsHandle;
			(void)position;
			(void)colour;
			(void)opacity;
			(void)textShapingContext;
			(void)meshList;
			return static_cast<int>(string.size()) * m_metrics.size / 2;
		}

		int GetVersion(Rml::FontFaceHandle handle) override
		{
			(void)handle;
			return 1;
		}

		void ReleaseFontResources() override {}

	private:
		Rml::FontMetrics m_metrics{ 16, 12.0f, 4.0f, 19.0f, 8.0f, 1.0f, 1.0f, false };
	};

	TitleRmlFontEngine TitleRmlFontEngineInstance;

	constexpr const char* TitleRml = R"(
<rml>
<head>
	<style>
		body {
			width: 100vw;
			height: 100vh;
			margin: 0;
			padding: 0;
		}
		#probe {
			position: absolute;
			left: 32px;
			top: 32px;
			width: 300px;
			height: 70px;
			background-color: rgb(34, 96, 55);
			border-width: 4px;
			border-color: rgb(121, 255, 166);
			padding: 0;
		}
		#probe:hover {
			background-color: rgb(45, 132, 74);
			border-color: rgb(255, 220, 99);
		}
		#menu {
			position: absolute;
			left: 430px;
			top: 418px;
			width: 420px;
		}
		button {
			display: block;
			width: 420px;
			height: 58px;
			margin-bottom: 18px;
			background-color: rgb(13, 49, 71);
			border-width: 2px;
			border-color: rgb(82, 215, 255);
			padding: 0;
		}
		button:hover {
			background-color: rgb(20, 88, 120);
			border-color: rgb(255, 213, 111);
		}
		#exit {
			background-color: rgb(21, 35, 52);
			border-color: rgb(109, 129, 146);
		}
	</style>
</head>
<body>
	<button id="probe"></button>
	<div id="menu">
		<button id="start"></button>
		<button id="exit"></button>
	</div>
</body>
</rml>
)";
}

TitleScene::TitleScene() = default;

TitleScene::~TitleScene()
{
	Unload();
}

void TitleScene::Load(const SceneLoadContext& context)
{
	m_width = context.width;
	m_height = context.height;
	m_batch.Initialize(context.device, 4096);
	InitializeRmlUi();
	RebuildBatch();
}

void TitleScene::Unload()
{
	ShutdownRmlUi();
}

void TitleScene::Update(float deltaTime, const Input& input)
{
	m_elapsedTime += deltaTime;
	m_probeButtonFlashTime = std::max(0.0f, m_probeButtonFlashTime - deltaTime);

	if (m_rmlContext != nullptr && input.IsMouseInsideClient())
	{
		m_rmlContext->ProcessMouseMove(input.GetMouseX(), input.GetMouseY(), 0);
		if (input.WasLeftMousePressed())
		{
			m_rmlContext->ProcessMouseButtonDown(0, 0);
		}
		if (input.WasLeftMouseReleased())
		{
			m_rmlContext->ProcessMouseButtonUp(0, 0);
		}
	}

	const float width = static_cast<float>(m_width);
	const float height = static_cast<float>(m_height);
	const float buttonLeft = width * 0.5f - 210.0f;
	const float buttonTop = height * 0.58f;
	const bool clickedProbeButton =
		input.WasLeftMousePressed() &&
		input.IsMouseInsideClient() &&
		static_cast<float>(input.GetMouseX()) >= ProbeButtonX &&
		static_cast<float>(input.GetMouseX()) <= ProbeButtonX + ProbeButtonWidth &&
		static_cast<float>(input.GetMouseY()) >= ProbeButtonY &&
		static_cast<float>(input.GetMouseY()) <= ProbeButtonY + ProbeButtonHeight;
	const bool clickedStartButton =
		input.WasLeftMousePressed() &&
		input.IsMouseInsideClient() &&
		static_cast<float>(input.GetMouseX()) >= buttonLeft &&
		static_cast<float>(input.GetMouseX()) <= buttonLeft + 420.0f &&
		static_cast<float>(input.GetMouseY()) >= buttonTop &&
		static_cast<float>(input.GetMouseY()) <= buttonTop + 58.0f;

	if (clickedProbeButton)
	{
		++m_probeButtonClickCount;
		m_probeButtonFlashTime = 0.35f;
		if (m_rmlDocument != nullptr)
		{
			if (Rml::Element* probeButton = m_rmlDocument->GetElementById("probe"))
			{
				probeButton->SetProperty("background-color", "rgb(128, 82, 24)");
				probeButton->SetProperty("border-color", "rgb(255, 220, 99)");
			}
		}
	}
	else if (m_probeButtonFlashTime <= 0.0f && m_rmlDocument != nullptr)
	{
		if (Rml::Element* probeButton = m_rmlDocument->GetElementById("probe"))
		{
			probeButton->SetProperty("background-color", "rgb(34, 96, 55)");
			probeButton->SetProperty("border-color", "rgb(121, 255, 166)");
		}
	}

	if (input.WasPressed(InputKey::Enter) || input.WasPressed(InputKey::Space) || clickedStartButton)
	{
		m_startRequested = true;
	}

	RebuildBatch();
}

void TitleScene::Render(Dx12Renderer& renderer) const
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
	RenderRmlUiToBatch();
	if (m_rmlDocument == nullptr)
	{
		m_batch.DrawText("RML LOAD FAILED", 32.0f, 96.0f, 2.0f, PromptColor);
		m_batch.DrawText(TitleRmlSystem.lastMessage.substr(0, 46), 32.0f, 122.0f, 1.5f, SubtleTextColor);
	}
	else if (m_rmlRenderer.GetLastRenderedTriangleCount() == 0)
	{
		m_batch.DrawText("RML ZERO GEOMETRY", 32.0f, 96.0f, 2.0f, PromptColor);
	}
	m_batch.DrawText("RML BUTTON", ProbeButtonX + 28.0f, ProbeButtonY + 18.0f, 2.4f, RmlLabelColor);
	const std::string probeText = m_probeButtonClickCount > 0
		? "CLICKED " + std::to_string(m_probeButtonClickCount)
		: "CLICK ME";
	m_batch.DrawText(probeText, ProbeButtonX + 28.0f, ProbeButtonY + 44.0f, 1.8f, PromptColor);
	DrawCenteredText("START GAME", height * 0.58f + 18.0f, 3.0f, RmlLabelColor);
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

void TitleScene::InitializeRmlUi()
{
	Rml::SetSystemInterface(&TitleRmlSystem);
	Rml::SetFontEngineInterface(&TitleRmlFontEngineInstance);
	if (!Rml::Initialise())
	{
		return;
	}

	m_rmlInitialized = true;
	m_rmlContext = Rml::CreateContext("title", Rml::Vector2i(static_cast<int>(m_width), static_cast<int>(m_height)), &m_rmlRenderer);
	if (m_rmlContext == nullptr)
	{
		return;
	}

	m_rmlDocument = m_rmlContext->LoadDocumentFromMemory(TitleRml, "title-screen.rml");
	if (m_rmlDocument != nullptr)
	{
		m_rmlDocument->Show();
	}
}

void TitleScene::ShutdownRmlUi()
{
	if (!m_rmlInitialized)
	{
		return;
	}

	if (m_rmlContext != nullptr)
	{
		m_rmlContext->UnloadAllDocuments();
		Rml::RemoveContext(m_rmlContext->GetName());
		m_rmlContext = nullptr;
		m_rmlDocument = nullptr;
	}

	Rml::Shutdown();
	m_rmlInitialized = false;
}

void TitleScene::RenderRmlUiToBatch()
{
	if (m_rmlContext == nullptr || m_rmlDocument == nullptr)
	{
		return;
	}

	m_rmlContext->Update();
	m_rmlRenderer.Begin(m_batch);
	m_rmlContext->Render();
	m_rmlRenderer.End();
}
