#pragma once

#include "Framework/Effects/IEffectService.h"

#include <DirectXMath.h>

#include <memory>
#include <string_view>

class Dx12Renderer;

class EffekseerEffectSystem final : public IEffectCatalog, public IEffectPlayer
{
public:
	EffekseerEffectSystem();
	~EffekseerEffectSystem();
	EffekseerEffectSystem(EffekseerEffectSystem&&) noexcept;
	EffekseerEffectSystem& operator=(EffekseerEffectSystem&&) noexcept;
	EffekseerEffectSystem(const EffekseerEffectSystem&) = delete;
	EffekseerEffectSystem& operator=(const EffekseerEffectSystem&) = delete;

	void Initialize(Dx12Renderer& renderer);
	void RegisterEffect(std::string_view id, std::string_view assetPath) override;
	void Play(std::string_view id, const DirectX::XMFLOAT3& position, float scale = 1.0f) override;
	void Update(float deltaTime);
	void Render(
		Dx12Renderer& renderer,
		const DirectX::XMMATRIX& view,
		const DirectX::XMMATRIX& projection);

	bool IsInitialized() const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
