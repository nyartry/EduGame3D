#pragma once

#include <Windows.h>
#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>

#ifdef GAME_NATIVE_TESTS
#include <CppUnitTest.h>
#endif

namespace TestSupport
{
	// Locate this test module, including when Visual Studio loads it into testhost.
	inline const char moduleAnchor{};
	inline std::filesystem::path ModuleDirectory()
	{
		HMODULE module{};
		if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCWSTR>(&moduleAnchor), &module))
			throw std::runtime_error("Cannot locate the test module");
		wchar_t path[32768]{};
		const auto length = GetModuleFileNameW(module, path, 32768);
		if (!length || length >= 32768) throw std::runtime_error("Cannot locate the test module directory");
		return std::filesystem::path(path).parent_path();
	}

	class RepositoryDirectory
	{
	public:
		RepositoryDirectory() : m_previous(std::filesystem::current_path())
		{
			auto directory = ModuleDirectory();
			while (!directory.empty())
			{
				if (std::filesystem::exists(directory / "EduGame3D.sln"))
				{
					std::filesystem::current_path(directory);
					return;
				}
				const auto parent = directory.parent_path();
				if (parent == directory) break;
				directory = parent;
			}
			throw std::runtime_error("Cannot find EduGame3D.sln above the test module");
		}
		~RepositoryDirectory()
		{
			std::error_code ignored;
			std::filesystem::current_path(m_previous, ignored);
		}
		RepositoryDirectory(const RepositoryDirectory&) = delete;
		RepositoryDirectory& operator=(const RepositoryDirectory&) = delete;
	private:
		std::filesystem::path m_previous;
	};

	enum class Category { Cpu, Gpu };
	struct TestCase
	{
		const char* label;
		void (*function)();
		Category category;
	};

	inline int RunConsole(int argc, char** argv, std::initializer_list<TestCase> cases)
	{
		bool gpu = false;
		for (int index = 1; index < argc; ++index) gpu |= std::string_view(argv[index]) == "--gpu";
		int failures = 0;
		for (const auto& test : cases)
		{
			if (test.category == Category::Gpu && !gpu) continue;
			try { test.function(); std::cout << "PASS " << test.label << '\n'; }
			catch (const std::exception& error) { ++failures; std::cerr << "FAIL " << test.label << ": " << error.what() << '\n'; }
			catch (...) { ++failures; std::cerr << "FAIL " << test.label << ": unknown exception\n"; }
		}
		return failures == 0 ? 0 : 1;
	}

#ifdef GAME_NATIVE_TESTS
	// Fixtures use process-wide cwd, diagnostic sinks and graphics caches.
	// Keep those fixtures isolated even if the IDE requests parallel methods.
	inline std::mutex nativeTestMutex;
	inline std::wstring WideMessage(const char* message)
	{
		const int length = MultiByteToWideChar(CP_UTF8, 0, message, -1, nullptr, 0);
		if (!length) return L"Test failed (message conversion failed)";
		std::wstring text(static_cast<std::size_t>(length), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, message, -1, text.data(), length);
		text.pop_back();
		return text;
	}
	inline void RunNative(void (*test)())
	{
		std::lock_guard lock(nativeTestMutex);
		try
		{
			RepositoryDirectory directory;
			test();
		}
		catch (const std::exception& error)
		{
			Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(WideMessage(error.what()).c_str());
		}
		catch (...)
		{
			Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(L"Unknown exception escaped the test");
		}
	}
#endif
}

// A single case list drives both console execution and native discovery.
#ifdef GAME_NATIVE_TESTS
#define GAME_TEST_CATEGORY_Cpu L"CPU"
#define GAME_TEST_CATEGORY_Gpu L"GPU"
#define GAME_NATIVE_CASE(function, label, category) \
	BEGIN_TEST_METHOD_ATTRIBUTE(function) \
		TEST_METHOD_ATTRIBUTE(L"Category", GAME_TEST_CATEGORY_##category) \
		TEST_DESCRIPTION(L##label) \
	END_TEST_METHOD_ATTRIBUTE() \
	TEST_METHOD(function) { TestSupport::RunNative(&::function); }
#define GAME_TEST_SUITE(suite, cases) \
	namespace VisualStudioTests { TEST_CLASS(suite) { public: cases(GAME_NATIVE_CASE) }; }
#else
#define GAME_CONSOLE_CASE(function, label, category) { label, &::function, TestSupport::Category::category },
#define GAME_TEST_SUITE(suite, cases) \
	int main(int argc, char** argv) { return TestSupport::RunConsole(argc, argv, { cases(GAME_CONSOLE_CASE) }); }
#endif
