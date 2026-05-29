#pragma once

#include "Rendering/Sprites/SpriteBatch.h"
#include "Scene/Core/IScene.h"

#include <Windows.h>

#include <DirectXMath.h>
#include <memory>
#include <string>
#include <string_view>

class Dx12Renderer;
struct ID3D12Device;

namespace Rml
{
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
	struct ButtonRect
	{
		float x{};
		float y{};
		float width{};
		float height{};
	};

	class StartButtonListener;

	void InitializeRmlUi();
	void UpdateRmlInput(const Input& input);
	void UpdateStartButtonRect();
	void RebuildBatch();
	void DrawCenteredText(std::string_view text, float centerY, float pixelSize, const DirectX::XMFLOAT4& color);

	SpriteBatch m_batch;
	Rml::Context* m_rmlContext{};
	Rml::ElementDocument* m_rmlDocument{};
	std::unique_ptr<StartButtonListener> m_startButtonListener;
	ButtonRect m_startButtonRect;
	UINT m_width{};
	UINT m_height{};
	float m_elapsedTime{};
	bool m_startRequested{};
	bool m_startButtonHovered{};
	bool m_startButtonPressed{};
	bool m_loggedStartButtonLayout{};
};
