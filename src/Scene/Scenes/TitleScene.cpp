#include "Scene/Scenes/TitleScene.h"

#include "Rendering/Core/Dx12Renderer.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/RenderInterface.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
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
	constexpr XMFLOAT4 ButtonTextColor{ 0.05f, 0.07f, 0.09f, 1.0f };

	void LogTitleRmlError(const wchar_t* message)
	{
		OutputDebugStringW(L"[TitleScene][RmlUi] ");
		OutputDebugStringW(message);
		OutputDebugStringW(L"\n");
	}

	class SpriteRmlRenderInterface final : public Rml::RenderInterface
	{
	public:
		struct Geometry
		{
			std::vector<Rml::Vertex> vertices;
			std::vector<int> indices;
		};

		void SetBatch(SpriteBatch* batch)
		{
			m_batch = batch;
		}

		Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override
		{
			auto geometry = std::make_unique<Geometry>();
			geometry->vertices.assign(vertices.begin(), vertices.end());
			geometry->indices.assign(indices.begin(), indices.end());
			return reinterpret_cast<Rml::CompiledGeometryHandle>(geometry.release());
		}

		void RenderGeometry(Rml::CompiledGeometryHandle geometryHandle, Rml::Vector2f translation, Rml::TextureHandle) override
		{
			if (m_batch == nullptr || geometryHandle == 0)
			{
				return;
			}

			const auto& geometry = *reinterpret_cast<Geometry*>(geometryHandle);
			for (size_t index = 0; index + 2 < geometry.indices.size(); index += 3)
			{
				const Rml::Vertex& a = geometry.vertices[static_cast<size_t>(geometry.indices[index])];
				const Rml::Vertex& b = geometry.vertices[static_cast<size_t>(geometry.indices[index + 1])];
				const Rml::Vertex& c = geometry.vertices[static_cast<size_t>(geometry.indices[index + 2])];
				const Rml::ColourbPremultiplied color = a.colour;
				m_batch->DrawTriangle(
					{ a.position.x + translation.x, a.position.y + translation.y },
					{ b.position.x + translation.x, b.position.y + translation.y },
					{ c.position.x + translation.x, c.position.y + translation.y },
					{
						static_cast<float>(color.red) / 255.0f,
						static_cast<float>(color.green) / 255.0f,
						static_cast<float>(color.blue) / 255.0f,
						static_cast<float>(color.alpha) / 255.0f
					});
			}
		}

		void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override
		{
			delete reinterpret_cast<Geometry*>(geometry);
		}

		Rml::TextureHandle LoadTexture(Rml::Vector2i& textureDimensions, const Rml::String&) override
		{
			textureDimensions = {};
			return {};
		}
		Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte>, Rml::Vector2i) override { return {}; }
		void ReleaseTexture(Rml::TextureHandle) override {}
		void EnableScissorRegion(bool) override {}
		void SetScissorRegion(Rml::Rectanglei) override {}

	private:
		SpriteBatch* m_batch{};
	};

	SpriteRmlRenderInterface& GetTitleRmlRenderInterface()
	{
		static SpriteRmlRenderInterface renderInterface;
		return renderInterface;
	}

	bool EnsureRmlUiInitialized()
	{
		static const bool initialized = []()
		{
			Rml::SetRenderInterface(&GetTitleRmlRenderInterface());
			return Rml::Initialise();
		}();
		return initialized;
	}

}

class TitleScene::StartButtonListener final : public Rml::EventListener
{
public:
	explicit StartButtonListener(bool& startRequested)
		: m_startRequested(startRequested)
	{
	}

	void ProcessEvent(Rml::Event& event) override
	{
		if (event == "click")
		{
			m_startRequested = true;
		}
	}

private:
	bool& m_startRequested;
};

TitleScene::TitleScene() = default;

TitleScene::~TitleScene()
{
	Unload();
}

void TitleScene::Load(const SceneLoadContext& context)
{
	m_width = context.width;
	m_height = context.height;
	m_loggedStartButtonLayout = false;
	m_batch.Initialize(context.device, 4096);
	InitializeRmlUi();
	RebuildBatch();
}

void TitleScene::Unload()
{
	if (m_rmlContext != nullptr)
	{
		Rml::RemoveContext("TitleScene");
		m_rmlContext = nullptr;
		m_rmlDocument = nullptr;
	}
	m_startButtonListener.reset();
}

void TitleScene::Update(float deltaTime, const Input& input)
{
	m_elapsedTime += deltaTime;
	UpdateRmlInput(input);

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

void TitleScene::InitializeRmlUi()
{
	if (!EnsureRmlUiInitialized())
	{
		LogTitleRmlError(L"Rml::Initialise failed.");
		return;
	}

	Rml::RemoveContext("TitleScene");
	m_rmlContext = Rml::CreateContext("TitleScene", { static_cast<int>(m_width), static_cast<int>(m_height) });
	if (m_rmlContext == nullptr)
	{
		LogTitleRmlError(L"Rml::CreateContext returned nullptr.");
		return;
	}

	const Rml::String document = R"(
<rml>
<head>
	<style>
		body {
			width: 100%;
			height: 100%;
			margin: 0px;
		}
		#start-button {
			position: absolute;
			left: 0px;
			top: 0px;
			width: 260px;
			height: 72px;
			display: inline-block;
			background-color: #e19428;
			border-width: 3px;
			border-color: #ffd568;
		}
		#start-button:hover {
			background-color: #ffb33d;
		}
		#start-button:active {
			background-color: #b86b19;
		}
	</style>
</head>
<body>
	<input type="button" id="start-button" value="START" />
</body>
</rml>
)";

	m_rmlDocument = m_rmlContext->LoadDocumentFromMemory(document);
	if (m_rmlDocument == nullptr)
	{
		LogTitleRmlError(L"LoadDocumentFromMemory returned nullptr.");
		return;
	}

	m_startButtonListener = std::make_unique<StartButtonListener>(m_startRequested);
	if (Rml::Element* button = m_rmlDocument->GetElementById("start-button"))
	{
		const int buttonWidth = 260;
		const int buttonHeight = 72;
		const int buttonLeft = static_cast<int>(m_width) / 2 - buttonWidth / 2;
		const int buttonTop = static_cast<int>(static_cast<float>(m_height) * 0.65f) - buttonHeight / 2;
		button->SetProperty("left", std::to_string(buttonLeft) + "px");
		button->SetProperty("top", std::to_string(buttonTop) + "px");
		button->SetProperty("width", std::to_string(buttonWidth) + "px");
		button->SetProperty("height", std::to_string(buttonHeight) + "px");
		button->AddEventListener("click", m_startButtonListener.get());
	}
	else
	{
		LogTitleRmlError(L"Could not find #start-button after loading the RML document.");
	}

	m_rmlDocument->Show();
	m_rmlContext->Update();
	UpdateStartButtonRect();
}

void TitleScene::UpdateRmlInput(const Input& input)
{
	if (m_rmlContext == nullptr)
	{
		return;
	}

	if (input.IsMouseInsideClient())
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
	else
	{
		m_rmlContext->ProcessMouseLeave();
	}

	m_rmlContext->Update();
	UpdateStartButtonRect();
	const float mouseX = static_cast<float>(input.GetMouseX());
	const float mouseY = static_cast<float>(input.GetMouseY());
	m_startButtonHovered =
		input.IsMouseInsideClient() &&
		mouseX >= m_startButtonRect.x &&
		mouseY >= m_startButtonRect.y &&
		mouseX < m_startButtonRect.x + m_startButtonRect.width &&
		mouseY < m_startButtonRect.y + m_startButtonRect.height;
	m_startButtonPressed = m_startButtonHovered && input.IsLeftMouseDown();
	if (m_startButtonHovered && input.WasLeftMouseReleased())
	{
		if (m_rmlDocument != nullptr)
		{
			if (Rml::Element* button = m_rmlDocument->GetElementById("start-button"))
			{
				button->Click();
				return;
			}
		}
		LogTitleRmlError(L"Click was inside the cached button rect, but #start-button was not found.");
	}
}

void TitleScene::UpdateStartButtonRect()
{
	if (m_rmlDocument == nullptr)
	{
		return;
	}

	Rml::Element* button = m_rmlDocument->GetElementById("start-button");
	if (button == nullptr)
	{
		LogTitleRmlError(L"Could not update layout: #start-button was not found.");
		return;
	}

	m_startButtonRect = {
		button->GetAbsoluteLeft(),
		button->GetAbsoluteTop(),
		button->GetOffsetWidth(),
		button->GetOffsetHeight()
	};
	if (m_startButtonRect.width <= 0.0f || m_startButtonRect.height <= 0.0f)
	{
		LogTitleRmlError(L"Invalid #start-button layout: width or height is zero.");
	}
	else if (!m_loggedStartButtonLayout)
	{
		wchar_t message[192]{};
		std::swprintf(
			message,
			sizeof(message) / sizeof(message[0]),
			L"#start-button layout x=%.1f y=%.1f width=%.1f height=%.1f",
			m_startButtonRect.x,
			m_startButtonRect.y,
			m_startButtonRect.width,
			m_startButtonRect.height);
		LogTitleRmlError(message);
		m_loggedStartButtonLayout = true;
	}
}

void TitleScene::RebuildBatch()
{
	const float width = static_cast<float>(m_width);
	const float height = static_cast<float>(m_height);
	const float pulse = (std::sin(m_elapsedTime * 3.2f) + 1.0f) * 0.5f;
	const float titleSize = std::clamp(width / 118.0f, 4.0f, 8.0f);
	const float subtitleSize = std::clamp(width / 260.0f, 2.0f, 4.0f);
	const float buttonTextSize = std::clamp(width / 230.0f, 3.0f, 5.2f);
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

	if (m_startButtonRect.width <= 0.0f || m_startButtonRect.height <= 0.0f)
	{
		m_batch.Upload();
		return;
	}

	const float pressOffset = m_startButtonPressed ? 2.0f : 0.0f;
	m_batch.DrawRectangle(m_startButtonRect.x, m_startButtonRect.y + 6.0f, m_startButtonRect.width, m_startButtonRect.height, { 0.03f, 0.04f, 0.05f, 0.52f });

	GetTitleRmlRenderInterface().SetBatch(&m_batch);
	if (m_rmlContext != nullptr)
	{
		m_rmlContext->Render();
	}
	GetTitleRmlRenderInterface().SetBatch(nullptr);

	m_batch.DrawRectangle(
		m_startButtonRect.x + 8.0f,
		m_startButtonRect.y + 8.0f + pressOffset,
		m_startButtonRect.width - 16.0f,
		3.0f,
		{ 1.0f, 0.90f, 0.48f, 0.72f });

	const XMFLOAT2 labelSize = m_batch.MeasureText("START", buttonTextSize);
	m_batch.DrawText(
		"START",
		m_startButtonRect.x + (m_startButtonRect.width - labelSize.x) * 0.5f,
		m_startButtonRect.y + pressOffset + (m_startButtonRect.height - labelSize.y) * 0.5f,
		buttonTextSize,
		ButtonTextColor);

	m_batch.Upload();
}

void TitleScene::DrawCenteredText(std::string_view text, float centerY, float pixelSize, const XMFLOAT4& color)
{
	const XMFLOAT2 textSize = m_batch.MeasureText(text, pixelSize);
	const float x = (static_cast<float>(m_width) - textSize.x) * 0.5f;
	const float y = centerY - textSize.y * 0.5f;
	m_batch.DrawText(text, x, y, pixelSize, color);
}
