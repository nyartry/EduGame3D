#include "TestSupport.h"
#include "Framework/Common/Common.h"
#include "Framework/Core/Diagnostics/Diagnostics.h"
#include "Framework/Core/Diagnostics/ExceptionUtils.h"

#include <exception>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>

namespace
{
	void Require(bool condition, const char* message)
	{
		if (!condition) throw std::runtime_error(message);
	}

	struct ResetDiagnosticSink
	{
		~ResetDiagnosticSink() { Diagnostics::SetSink({}); }
	};

	void HResultSuccessIsSilent()
	{
		int writes = 0;
		ResetDiagnosticSink reset;
		Diagnostics::SetSink([&](std::string_view) { ++writes; });
		ThrowIfFailed(S_OK, "Successful operation");
		ThrowIfFailed(S_FALSE, "Successful operation with status");
		Require(writes == 0, "Successful HRESULTs must not produce diagnostics");
	}

	void HResultFailureHasContext()
	{
		int writes = 0;
		ResetDiagnosticSink reset;
		Diagnostics::SetSink([&](std::string_view) { ++writes; });
		const auto location = std::source_location::current();
		std::string message;
		try { ThrowIfFailed(E_FAIL, "Create test resource", location); }
		catch (const std::runtime_error& error) { message = error.what(); }
		Require(message.find("Create test resource failed: HRESULT 0x80004005") != std::string::npos,
			"Failed HRESULT must throw with operation and hexadecimal HRESULT");
		Require(message.find(std::string(location.file_name()) + ':' + std::to_string(location.line())) != std::string::npos,
			"Failed HRESULT must retain the caller's file and line");
		Require(writes == 0, "ThrowIfFailed must leave reporting to the recovery or application boundary");
	}

	void HResultUsesCallerWhenOperationIsEmpty()
	{
		const auto location = std::source_location::current();
		std::string message;
		try { ThrowIfFailed(E_ACCESSDENIED, {}, location); }
		catch (const std::runtime_error& error) { message = error.what(); }
		Require(message.find(location.function_name()) != std::string::npos &&
			message.find("0x80070005") != std::string::npos,
			"Unnamed operations must preserve caller function and HRESULT");
	}

	void ExceptionDescriptionsPreserveCause()
	{
		Require(DescribeException() == "No exception" && DescribeException(nullptr) == "No exception",
			"An empty exception pointer must be safe to describe");
		Require(DescribeException(std::make_exception_ptr(std::runtime_error("original error"))) == "original error",
			"Explicit exception pointers must retain the original message");
		try { throw std::logic_error("current error"); }
		catch (...) { Require(DescribeException() == "current error", "Default argument must capture the active exception"); }
		try { throw 42; }
		catch (...) { Require(DescribeException() == "Unknown exception", "Nonstandard exceptions need a usable fallback"); }
		try
		{
			try { throw std::runtime_error("inner error"); }
			catch (...) { std::throw_with_nested(std::runtime_error("outer operation")); }
		}
		catch (...)
		{
			Require(DescribeException() == "outer operation\nCaused by: inner error",
				"Nested exceptions must retain both operation context and original cause");
		}
		struct NonstandardError {};
		try
		{
			try { throw std::runtime_error("inner cause"); }
			catch (...) { std::throw_with_nested(NonstandardError{}); }
		}
		catch (...)
		{
			Require(DescribeException() == "Unknown exception\nCaused by: inner cause",
				"A nonstandard nested exception must still retain its original cause");
		}
	}

	void ExceptionDescriptionsHandleEmptyAndDeepNesting()
	{
		struct DetachedNestedError : std::runtime_error, std::nested_exception
		{
			DetachedNestedError() : std::runtime_error("detached error") {}
		};
		const auto detached = std::make_exception_ptr(DetachedNestedError{});
		Require(DescribeException(detached) == "detached error",
			"A nested_exception with no captured cause must not terminate the process");

		auto error = std::make_exception_ptr(std::runtime_error("innermost"));
		for (unsigned depth = 0; depth < 64; ++depth)
		{
			try
			{
				try { std::rethrow_exception(error); }
				catch (...) { std::throw_with_nested(std::runtime_error("wrapper")); }
			}
			catch (...) { error = std::current_exception(); }
		}
		const auto description = DescribeException(error);
		Require(description.starts_with("wrapper") && description.find("nesting limit") != std::string::npos &&
			description.size() < 1024, "Deep exception nesting must stop with an explicit truncation marker");
	}

	void BoundaryReportsOnceAndKeepsOriginalError()
	{
		int writes = 0;
		std::string diagnostic;
		ResetDiagnosticSink reset;
		Diagnostics::SetSink([&](std::string_view message) { ++writes; diagnostic = message; });
		std::exception_ptr original;
		try { ThrowIfFailed(E_FAIL, "Boundary test operation"); }
		catch (...) { original = std::current_exception(); Diagnostics::Write(DescribeException()); }
		Require(original && writes == 1 && diagnostic.find("Boundary test operation") != std::string::npos,
			"A handled failure must be recorded exactly once at the boundary");

		Diagnostics::SetSink([&](std::string_view) { ++writes; throw std::runtime_error("broken diagnostic sink"); });
		try
		{
			try { std::rethrow_exception(original); }
			catch (...) { Diagnostics::Write(DescribeException()); throw; }
		}
		catch (const std::runtime_error& error)
		{
			Require(error.what() == diagnostic && writes == 2,
				"A throwing diagnostic sink must not replace the original exception");
		}
	}
}

#define DIAGNOSTICSTESTS_CASES(TEST) \
	TEST(HResultSuccessIsSilent, "HResultSuccessIsSilent", Cpu) \
	TEST(HResultFailureHasContext, "HResultFailureHasContext", Cpu) \
	TEST(HResultUsesCallerWhenOperationIsEmpty, "HResultUsesCallerWhenOperationIsEmpty", Cpu) \
	TEST(ExceptionDescriptionsPreserveCause, "ExceptionDescriptionsPreserveCause", Cpu) \
	TEST(ExceptionDescriptionsHandleEmptyAndDeepNesting, "ExceptionDescriptionsHandleEmptyAndDeepNesting", Cpu) \
	TEST(BoundaryReportsOnceAndKeepsOriginalError, "BoundaryReportsOnceAndKeepsOriginalError", Cpu)

GAME_TEST_SUITE(DiagnosticsTests, DIAGNOSTICSTESTS_CASES)
