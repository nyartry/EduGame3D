#pragma once

class Input;
class IRenderDevice;
class IRenderer;
class ModelAssetCache;

class Actor
{
public:
	virtual ~Actor() = default;

	// Scene-owned CPU preparation; no GPU or UI access on this worker thread.
	virtual void Prepare(ModelAssetCache&) {}
	virtual void Initialize(IRenderDevice& device) = 0;
	virtual void Update(float deltaTime, const Input& input) = 0;
	virtual void Draw(IRenderer& renderer) const = 0;
};
