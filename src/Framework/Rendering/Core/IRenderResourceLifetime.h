#pragma once

#include <functional>

// Owns deferred destruction that must wait until submitted (or currently recorded)
// GPU work no longer references a resource. The renderer decides when it is safe.
class IRenderResourceLifetime
{
public:
	virtual ~IRenderResourceLifetime() = default;
	virtual void DeferRelease(std::function<void()> release) = 0;
};
