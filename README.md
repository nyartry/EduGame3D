# EduGame3D

授業用3Dゲームフレームワーク。日本国内の教育機関の通常授業で、3Dゲームのプログラミングを学ぶためのプロジェクトです。

**開発を応援する：[GitHub Sponsors](https://github.com/sponsors/nyartry)** — 単発・月額の任意支援を受け付けています。[支援について](SUPPORT.md)

A C++ / DirectX 12 framework for classroom game programming on Windows, built with Visual Studio 2022.

## 利用条件・授業で利用する方へ

著作者・権利者表示: **Haruyuki Ishinaka**。本プロジェクトの権利者が保有する部分には、独自の [EduGame3D 利用条件](LICENSE) を適用します。

- **学習・開発**: 対象部分の入手、手元でのビルド・実行・改変は無償で許可します。
- **制作したゲームの公開・販売**: 対象部分を含む実行物・ソース等は、無料公開やポートフォリオの配布も含め、事前の個別相談と許可が必要です。法令上の例外とGitHub規約に基づく閲覧・fork等は維持します。
- **授業での配信**: コードや解説をLMS・オンライン講義等で送信する場合は、学校のSARTRAS担当者に確認し、学校の利用報告の対象となるEduGame3Dの利用を報告してください。**GitHubの全利用者に一律の報告義務があるという意味ではありません。**

手順と相談先は [授業利用・SARTRAS報告ガイド](docs/sartras-user-guide.md)、報告に使う作品名・名義・使用版は [作品情報](docs/work-identification.md) にまとめています。第三者コード・素材は個別条件が優先します。音源・エフェクト・画像等の未確認事項は [権利台帳](docs/rights-inventory.md) を参照してください。

These are custom terms: local learning and development are permitted for the covered portions; publishing, distributing or selling games containing them requires prior individual permission, subject to statutory exceptions and GitHub's terms. See [LICENSE](LICENSE). The SARTRAS guidance concerns qualifying classroom transmissions and the institution's reporting procedures.

## 開発への支援

EduGame3Dが授業やゲーム制作に役立ったら、[GitHub Sponsors](https://github.com/sponsors/nyartry)から開発費用へのご支援をお願いします。支援は完全に任意で、利用条件は変わりません。個別サポート・機能追加・更新の約束を伴うものではありません。単発・月額の支援先と詳細は [SUPPORT.md](SUPPORT.md) をご覧ください。

## Purpose and handoff

EduGame3D is intended for teaching, reading and modifying source code, and building students' own 3D games. It is designed for use across educational institutions in Japan. The framework supports different game genres without prescribing gameplay rules; the included game demonstrates the framework's APIs.

ソースコードを公開し、GitHub Sponsorsによる任意支援を受け付けています。素材・依存物の権利確認は継続中です。確認状況は [権利台帳](docs/rights-inventory.md)、SARTRASに関する過去の調査は [SARTRAS準備調査](docs/sartras-readiness-audit.md) を参照してください。SARTRASへの登録や分配対象への該当は未確認です。

See [学生配布用3Dゲームひな型のコア調査・引き継ぎ](docs/student-template-core-handoff.md) for the project intent, current implementation, proposed improvements, validation results, and instructions for continuing work on another PC.

## What is included

- Win32 window creation
- Direct3D 12 device, swap chain, command queue, command list
- Root signature and graphics pipeline state
- A perspective camera, depth buffer, and simple 3D ground mesh
- Visual Studio 2022 solution and project files
- EduHuman: an original articulated humanoid with idle, jog and kick animations, editable Blender source, and a procedural generation script

## Source layout

- `src/Game`: sample gameplay, scenes, actions, and content definitions.
- `src/Framework`: reusable engine APIs, runtime systems, and visible backend adapters.
- `src/Launcher`: Win32 composition root that connects Game to concrete engine implementations.
- `tools/AnimationEventEditor`: editor executable that reuses the same static engine library.

The solution builds `EngineFramework.lib` and `GameModule.lib` as source-visible static libraries. Third-party runtime dependencies include Assimp DLLs. See [docs/architecture.md](docs/architecture.md) for dependency rules and extension points.

## Requirements

- Windows 10 or later
- Visual Studio 2022 with the Desktop development with C++ workload
- Windows 10/11 SDK
- Git and Git LFS (required for third-party libraries and model/audio assets)

## Get the source

Git LFSをインストールしてから、次のコマンドで取得してください。ライブラリや素材の実体を取得するため、ビルド前に `git lfs pull` まで実行します。

```powershell
git lfs install
git clone https://github.com/nyartry/EduGame3D.git
cd EduGame3D
git lfs pull
```

## Build

Open `EduGame3D.sln` in Visual Studio 2022, then build and run `GameApp` with `Debug|x64`.

To run the built game outside Visual Studio, use the repository root as the working directory: `.\x64\Debug\GameApp.exe`.

## Character assets

The sample uses `Content/Models/EduHuman/`. Its mesh, skeleton and animations were generated from primitives and mathematical keyframes for this project, with Codex assistance; no external character mesh, texture or motion capture was used. Blender is only needed to regenerate or edit the assets, not to build or play the game. The two existing `Untitled` FBXs are retained as samples confirmed by the project owner to be their own work.

See [model assets](Content/Models/README.md) for editing and [replacement record](docs/model-replacement.md) for the scope and verification. The current source tree excludes the previous external character models. `tools/export_clean_source.ps1` creates a source folder and ZIP without Git history, checks known old model hashes, and includes the downloaded third-party dependencies with their existing notices. This export addresses the model replacement; the separate [rights inventory](docs/rights-inventory.md) still applies to other content and to publication.

## Player controls

- `W` / `A` / `S` / `D`: move
- `Space`: jump
- `X`: kick; press again between 0.5 and 1.2 seconds to queue the next kick

## Camera controls

- `1`: existing interactive follow camera (default)
- `2`: critically damped spring follow camera
- `3`: first-person camera
- `4`: orbit camera
- `5`: Catmull-Rom spline camera; press `5` again to restart the shot
- Arrow keys control the active interactive camera. Hold `Shift` with Up/Down to zoom where supported, and hold `Z` to align the view with the player.

## Dependency check

`tools/check_architecture.ps1` prevents Game from directly or transitively depending on Win32, DX12, RmlUi, Effekseer, Assimp, or other backend implementation types. It runs automatically when `EngineFramework` builds.

See [error handling and TDD](docs/error-handling.md) for failure recovery, diagnostic ownership, and regression test commands.

The solution also includes `VisualStudioTests`, a native C++ test project. Build it and open **Test > Test Explorer** to run or debug individual tests. See [Visual Studio testing](docs/visual-studio-tests.md) for setup and command-line equivalents.
