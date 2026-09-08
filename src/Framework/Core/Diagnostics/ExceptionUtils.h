#pragma once

#include <exception>
#include <string>
#include <utility>

// Describe at the boundary that recovers from or reports the failure.
inline std::string DescribeException(std::exception_ptr error = std::current_exception())
{
	if (!error) return "No exception";

	std::string description;
	// Bound malformed/cyclic chains as well as unusually deep nesting.
	constexpr unsigned maxDepth = 16;
	for (unsigned depth = 0; error && depth < maxDepth; ++depth)
	{
		if (depth != 0) description += "\nCaused by: ";
		const auto current = std::exchange(error, {});
		try { std::rethrow_exception(current); }
		catch (const std::exception& exception)
		{
			description += exception.what();
			if (const auto* nested = dynamic_cast<const std::nested_exception*>(&exception))
				error = nested->nested_ptr();
		}
		catch (const std::nested_exception& exception)
		{
			description += "Unknown exception";
			error = exception.nested_ptr();
		}
		catch (...) { description += "Unknown exception"; }
	}
	if (error) description += "\nCaused by: [exception nesting limit reached]";
	return description;
}
