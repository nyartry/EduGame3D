#pragma once

#include <Windows.h>

#include <memory>

namespace AnimationEventEditorTool
{
	class AnimationEventEditorApp
	{
	public:
		static constexpr UINT DefaultWindowWidth = 1600;
		static constexpr UINT DefaultWindowHeight = 900;

		AnimationEventEditorApp();
		~AnimationEventEditorApp();

		AnimationEventEditorApp(const AnimationEventEditorApp&) = delete;
		AnimationEventEditorApp& operator=(const AnimationEventEditorApp&) = delete;

		void Initialize(HWND hwnd);
		void Shutdown() noexcept;
		void Tick();

		void OpenFbx();
		void OpenEvents();
		void SaveDefaultEvents();
		void Undo();
		void Redo();
		bool CanUndo() const;
		bool CanRedo() const;
		void ResetLayout();

		void RequestResize(UINT width, UINT height);
		void RequestClientResize();

	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}
