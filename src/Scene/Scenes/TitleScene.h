#pragma once

#include "Rendering/Sprites/SpriteBatch.h"
#include "Scene/Core/IScene.h"

#include <Windows.h>

#include <DirectXMath.h>
#include <string>
#include <string_view>

class Dx12Renderer;
struct ID3D12Device;

class TitleScene : public IScene
{
public:
	void Load(const SceneLoadContext& context) override;
	void Update(float deltaTime, const Input& input) override;
	void Render(Dx12Renderer& renderer) const override;

	DirectX::XMMATRIX GetViewProjectionMatrix() const override;
	std::string GetRequestedSceneName() const override;
	bool ShouldLoadRequestedSceneAsync() const override;

private:
	void RebuildBatch();
	void DrawCenteredText(std::string_view text, float centerY, float pixelSize, const DirectX::XMFLOAT4& color);

	SpriteBatch m_batch;
	UINT m_width{};
	UINT m_height{};
	float m_elapsedTime{};
	bool m_acceptStartInput{};
	bool m_startRequested{};
};
