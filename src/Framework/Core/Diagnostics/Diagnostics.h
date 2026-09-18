#pragma once

#include <cstdio>
#include <functional>
#include <mutex>
#include <string_view>
#include <utility>

// Install sinks at the application boundary. Calls may originate in loaders.
class Diagnostics final
{
public:
	using Sink = std::function<void(std::string_view)>;
	static void SetSink(Sink sink)
	{
		std::lock_guard lock(s_mutex);
		s_sink = std::move(sink);
	}
	static void Write(std::string_view message) noexcept
	{
		try
		{
			Sink sink;
			{ std::lock_guard lock(s_mutex); sink = s_sink; }
			if (sink) sink(message);
			else { std::fwrite(message.data(), 1, message.size(), stderr); std::fputc('\n', stderr); }
		}
		catch (...) {} // Reporting must not replace the original error.
	}
private:
	inline static std::mutex s_mutex;
	inline static Sink s_sink;
};
