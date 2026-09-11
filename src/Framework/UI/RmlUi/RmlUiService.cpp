#include "Framework/UI/RmlUi/RmlUiService.h"

#include "Framework/Rendering/RmlUi/RmlUiSpriteRenderInterface.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/FontEngineInterface.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/SystemInterface.h>

#include <string>
#include <unordered_map>
#include <utility>

namespace
{
	class EngineRmlSystemInterface final : public Rml::SystemInterface
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

	class EngineRmlFontEngine final : public Rml::FontEngineInterface
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

	EngineRmlSystemInterface EngineRmlSystem;
	EngineRmlFontEngine EngineRmlFont;

	class ClickListener final : public Rml::EventListener
	{
	public:
		void ProcessEvent(Rml::Event&) override
		{
			m_clicked = true;
		}

		bool Consume()
		{
			return std::exchange(m_clicked, false);
		}

	private:
		bool m_clicked{};
	};

	class RmlUiDocument final : public IUiDocument
	{
	public:
		RmlUiDocument(std::string contextName, std::uint32_t width, std::uint32_t height, std::string_view markup)
			: m_contextName(std::move(contextName))
		{
			m_context = Rml::CreateContext(
				m_contextName,
				Rml::Vector2i(static_cast<int>(width), static_cast<int>(height)),
				&m_renderer);
			if (m_context == nullptr)
			{
				m_error = "RmlUi context creation failed.";
				return;
			}

			m_document = m_context->LoadDocumentFromMemory(Rml::String(markup), m_contextName + ".rml");
			if (m_document == nullptr)
			{
				m_error = EngineRmlSystem.lastMessage;
				return;
			}
			m_document->Show();
			m_context->Update();
		}

		~RmlUiDocument() override
		{
			if (m_context != nullptr)
			{
				m_context->UnloadAllDocuments();
				Rml::RemoveContext(m_contextName);
			}
		}

		void Resize(std::uint32_t width, std::uint32_t height) override
		{
			if (m_context == nullptr || width == 0 || height == 0)
			{
				return;
			}
			m_context->SetDimensions(Rml::Vector2i(static_cast<int>(width), static_cast<int>(height)));
			// Commit layout before this frame's first pointer event, not only when
			// RenderTo later runs. Otherwise the first click uses the old hit boxes.
			m_context->Update();
		}

		std::optional<UiElementBounds> GetElementBounds(std::string_view elementId) const override
		{
			if (m_document == nullptr)
			{
				return std::nullopt;
			}
			Rml::Element* element = m_document->GetElementById(Rml::String(elementId));
			if (element == nullptr)
			{
				return std::nullopt;
			}
			const auto offset = element->GetAbsoluteOffset(Rml::BoxArea::Border);
			const auto size = element->GetBox().GetSize(Rml::BoxArea::Border);
			return UiElementBounds{ offset.x, offset.y, size.x, size.y };
		}

		void ProcessPointerMove(int x, int y) override
		{
			if (m_context != nullptr)
			{
				m_context->ProcessMouseMove(x, y, 0);
			}
		}

		void ProcessPointerLeave() override
		{
			if (m_context != nullptr)
			{
				m_context->ProcessMouseLeave();
			}
		}

		void ProcessPointerButtonDown(int button) override
		{
			if (m_context != nullptr)
			{
				m_context->ProcessMouseButtonDown(button, 0);
			}
		}

		void ProcessPointerButtonUp(int button) override
		{
			if (m_context != nullptr)
			{
				m_context->ProcessMouseButtonUp(button, 0);
			}
		}

		void WatchClick(std::string_view elementId) override
		{
			if (m_document == nullptr)
			{
				return;
			}

			const std::string id(elementId);
			if (m_clickListeners.contains(id))
			{
				return;
			}
			if (Rml::Element* element = m_document->GetElementById(id))
			{
				auto listener = std::make_unique<ClickListener>();
				element->AddEventListener("click", listener.get());
				m_clickListeners.emplace(id, std::move(listener));
			}
		}

		bool ConsumeClick(std::string_view elementId) override
		{
			const auto iterator = m_clickListeners.find(std::string(elementId));
			return iterator != m_clickListeners.end() && iterator->second->Consume();
		}

		void SetElementClass(std::string_view elementId, std::string_view className, bool enabled) override
		{
			if (m_document == nullptr)
			{
				return;
			}
			if (Rml::Element* element = m_document->GetElementById(Rml::String(elementId)))
			{
				element->SetClass(Rml::String(className), enabled);
			}
		}

		void RenderTo(SpriteBatch& batch) override
		{
			if (m_context == nullptr || m_document == nullptr)
			{
				return;
			}
			m_context->Update();
			m_renderer.Begin(batch);
			m_context->Render();
			m_renderer.End();
		}

		bool IsLoaded() const override
		{
			return m_document != nullptr;
		}

		std::uint32_t GetLastRenderedTriangleCount() const override
		{
			return m_renderer.GetLastRenderedTriangleCount();
		}

		std::string GetLastError() const override
		{
			return m_error;
		}

	private:
		std::string m_contextName;
		std::string m_error;
		RmlUiSpriteRenderInterface m_renderer;
		Rml::Context* m_context{};
		Rml::ElementDocument* m_document{};
		std::unordered_map<std::string, std::unique_ptr<ClickListener>> m_clickListeners;
	};
}

RmlUiService::~RmlUiService()
{
	Shutdown();
}

bool RmlUiService::Initialize()
{
	if (m_initialized)
	{
		return true;
	}

	Rml::SetSystemInterface(&EngineRmlSystem);
	Rml::SetFontEngineInterface(&EngineRmlFont);
	m_initialized = Rml::Initialise();
	return m_initialized;
}

void RmlUiService::Shutdown()
{
	if (!m_initialized)
	{
		return;
	}
	Rml::Shutdown();
	m_initialized = false;
}

std::unique_ptr<IUiDocument> RmlUiService::CreateDocument(
	std::string_view name,
	std::uint32_t width,
	std::uint32_t height,
	std::string_view markup)
{
	if (!m_initialized && !Initialize())
	{
		return {};
	}

	std::string contextName(name);
	contextName += "-" + std::to_string(m_nextContextId++);
	return std::make_unique<RmlUiDocument>(std::move(contextName), width, height, markup);
}
