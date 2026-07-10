#pragma once

#include "Framework/Rendering/RmlUi/RmlUiSpriteRenderInterface.h"
#include "Framework/Rendering/Sprites/SpriteBatch.h"
#include "Framework/Scene/Core/IScene.h"

#include <RmlUi/Core/Types.h>

#include <Windows.h>

#include <DirectXMath.h>
#include <string>
#include <string_view>

class Dx12Renderer;
class IAudioService;
struct ID3D12Device;
namespace Rml {
class Context;
class ElementDocument;
}

class TitleScene : public IScene
{
public:
	TitleScene();
	~TitleScene() override;

	void Load(const SceneLoadContext& context) override;
	void Unload() override;
	void Update(float deltaTime, const Input& input) override;
	void Render(Dx12Renderer& renderer) const override;

	DirectX::XMMATRIX GetViewProjectionMatrix() const override;
	std::string GetRequestedSceneName() const override;
	bool ShouldLoadRequestedSceneAsync() const override;

private:
	void RebuildBatch();
	void DrawCenteredText(std::string_view text, float centerY, float pixelSize, const DirectX::XMFLOAT4& color);
	void InitializeRmlUi();
	void ShutdownRmlUi();
	void RenderRmlUiToBatch();

	SpriteBatch m_batch;
	IAudioService* m_audio{};
	RmlUiSpriteRenderInterface m_rmlRenderer;
	Rml::Context* m_rmlContext{};
	Rml::ElementDocument* m_rmlDocument{};
	UINT m_width{};
	UINT m_height{};
	float m_elapsedTime{};
	bool m_startRequested{};
	bool m_rmlInitialized{};
	int m_probeButtonClickCount{};
	float m_probeButtonFlashTime{};
	int m_selectedSampleButton{};
	int m_sampleButtonClickCount{};
};
