#pragma once

class Input;
enum class InputKey;

// Narrow write access for platform adapters. Game code only receives the
// read-only Input API, while backends do not need friendship by concrete name.
class InputWriter final
{
public:
	static void BeginFrame(Input& input);
	static void SetKey(Input& input, InputKey key, bool isDown);
	static void SetPointer(Input& input, bool leftButtonDown, bool insideClient, int x, int y);
};
