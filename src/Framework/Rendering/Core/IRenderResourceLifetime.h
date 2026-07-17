#pragma once

#include <functional>

// Owns deferred destruction that must wait until submitted GPU work no longer
// references a resource. The concrete renderer decides when the callback is safe.
class IRenderResourceLifetime
{
public:
	virtual ~IRenderResourceLifetime() = default;
	virtual void DeferRelease(std::function<void()> release) = 0;
};
