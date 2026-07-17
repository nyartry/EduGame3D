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

class TitleScene : public IScene
{
public:
	void Load(const SceneLoadContext& context) override;
	void Unload() override;
	void Update(float deltaTime, const Input& input) override;
	void Render(IRenderer& renderer) const override;

	DirectX::XMMATRIX GetViewProjectionMatrix() const override;
	std::string GetRequestedSceneName() const override;
	bool ShouldLoadRequestedSceneAsync() const override;

private:
	void RebuildBatch();
	void DrawCenteredText(std::string_view text, float centerY, float pixelSize, const DirectX::XMFLOAT4& color);

	SpriteBatch m_batch;
	IAudioService* m_audio{};
	std::unique_ptr<IUiDocument> m_uiDocument;
	std::uint32_t m_width{};
	std::uint32_t m_height{};
	float m_elapsedTime{};
	bool m_startRequested{};
	int m_probeButtonClickCount{};
	float m_probeButtonFlashTime{};
	int m_selectedSampleButton{};
	int m_sampleButtonClickCount{};
};
