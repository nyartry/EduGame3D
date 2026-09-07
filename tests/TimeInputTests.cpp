#include "Framework/Core/Time/FixedStepClock.h"
#include "Framework/Scene/Input/InputWriter.h"
#include "Framework/Scene/Input/SimulationInputBuffer.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

void Require(bool condition, const char* message)
{
	if (!condition) throw std::runtime_error(message);
}

int main()
{
	try
	{
		for (const int fps : {30, 60, 144})
		{
			FixedStepClock clock;
			unsigned updates = 0;
			double distance = 0;
			for (int frame = 0; frame < fps * 10; ++frame)
			{
				const auto count = clock.Advance(1.0 / fps);
				updates += count;
				for (unsigned step = 0; step < count; ++step) distance += 3.0 * clock.GetStepSeconds();
			}
			Require(updates == 600, "Render rate changed simulation step count");
			Require(std::abs(distance - 30.0) < 1.0e-5, "Render rate changed movement distance");
		}
		FixedStepClock clock;
		Require(clock.Advance(30.0) == 8, "Stall must have bounded catch-up");
		Require(clock.Advance(1.0 / 60) == 1, "Stall backlog must be discarded");
		Require(clock.Advance(-1) == 0 && clock.Advance(std::numeric_limits<double>::infinity()) == 0,
			"Invalid elapsed time must be ignored");
		clock.Advance(1.0 / 144);
		clock.Reset();
		Require(clock.Advance(1.0 / 144) == 0, "Resume must discard pending time");

		Input input;
		SimulationInputBuffer buffer;
		InputWriter::BeginFrame(input);
		InputWriter::SetKey(input, InputKey::Space, true);
		InputWriter::SetPointer(input, true, true, 12, 34);
		buffer.Capture(input); // No simulation update in this frame.
		InputWriter::BeginFrame(input);
		InputWriter::SetKey(input, InputKey::Space, false);
		InputWriter::SetPointer(input, false, true, 13, 35);
		buffer.Capture(input);
		const auto first = buffer.ConsumeStep();
		Require(first.WasPressed(InputKey::Space) && first.WasReleased(InputKey::Space), "Quick tap lost between steps");
		Require(first.WasPressed(InputKey::Space), "Reading an edge must not consume it");
		Require(first.WasLeftMousePressed() && first.WasLeftMouseReleased() && first.GetMouseX() == 13, "Pointer edges lost");
		for (int i = 0; i < 7; ++i)
		{
			const auto subsequent = buffer.ConsumeStep();
			Require(!subsequent.WasAnyPressed() && !subsequent.WasReleased(InputKey::Space) &&
				!subsequent.WasLeftMousePressed(), "Catch-up repeated an edge");
		}
		for (int tap = 0; tap < 2; ++tap)
		{
			InputWriter::BeginFrame(input);
			InputWriter::SetKey(input, InputKey::X, true);
			buffer.Capture(input);
			InputWriter::BeginFrame(input);
			InputWriter::SetKey(input, InputKey::X, false);
			buffer.Capture(input);
		}
		Require(buffer.ConsumeStep().WasPressed(InputKey::X) && buffer.ConsumeStep().WasPressed(InputKey::X),
			"Multiple queued taps must each be delivered");
		buffer.Reset();
		Require(!buffer.ConsumeStep().WasAnyPressed(), "Focus reset left stale input");
		std::cout << "Time/input regression tests passed.\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
