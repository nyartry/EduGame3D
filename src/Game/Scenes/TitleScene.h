#pragma once

#include "Framework/Rendering/Sprites/SpriteBatch.h"
#include "Framework/Scene/Core/IScene.h"
#include "Framework/UI/IUiService.h"

#include <DirectXMath.h>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

class IAudioService;
class IRenderDevice;

class TitleScene : public IScene
{
public:
	TitleScene(
		IRenderDevice& renderDevice,
		IAudioService& audio,
		IUiService& ui,
		std::uint32_t width,
		std::uint32_t height);

	void Activate() override;
	void Unload() override;
	void Update(float, const Input&) override {}
	void UpdateFrame(float deltaTime, const Input& input) override;
	void RenderOverlay(IRenderer& renderer) const override;

	RenderView GetRenderView() const override;
	std::string GetRequestedSceneName() const override;
	bool ShouldLoadRequestedSceneAsync() const override;
	void OnSceneLoadFailed() override { m_startRequested = false; m_loadFailed = true; }

private:
	void RebuildBatch();
	void DrawCenteredText(std::string_view text, float centerY, float pixelSize, const DirectX::XMFLOAT4& color);

	SpriteBatch m_batch;
	IRenderDevice& m_renderDevice;
	IAudioService& m_audio;
	IUiService& m_ui;
	std::unique_ptr<IUiDocument> m_uiDocument;
	std::uint32_t m_width{};
	std::uint32_t m_height{};
	float m_elapsedTime{};
	bool m_startRequested{};
	bool m_loadFailed{};
	int m_probeButtonClickCount{};
	float m_probeButtonFlashTime{};
	int m_selectedSampleButton{};
	int m_sampleButtonClickCount{};
};
