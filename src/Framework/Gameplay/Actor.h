#pragma once

class Input;
class IRenderDevice;
class IRenderer;

class Actor
{
public:
	virtual ~Actor() = default;

	virtual void Initialize(IRenderDevice& device) = 0;
	virtual void Update(float deltaTime, const Input& input) = 0;
	virtual void Draw(IRenderer& renderer) const = 0;
};
