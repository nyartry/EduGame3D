# Visual Studioでテストを実行する

Visual Studio 2022のMicrosoft Unit Testing Framework for C++へ12種類のテストを接続しています。`EduGame3D.sln` の `VisualStudioTests` プロジェクトが、87件のテストを含む専用DLLを生成します（2026-09-11時点）。ゲーム本体へテストは組み込まれません。

## 最初の実行

1. Visual Studioで `EduGame3D.sln` を開きます。既に開いていた場合は、ソリューションファイルの変更通知で再読み込みします。
2. 構成を `Debug`、プラットフォームを `x64` にします。
3. ソリューションエクスプローラーの `VisualStudioTests` を右クリックして「ビルド」を実行します。
4. メニューの「テスト」→「テスト エクスプローラー」を開きます。
5. `VisualStudioTests` の各クラスからテストを選択して実行します。「すべて実行」ではGPUのケースも含めて実行します。

テスト名、成功・失敗、実行時間、失敗した条件のメッセージを確認できます。テストを右クリックして「デバッグ」を選ぶと、C++のブレークポイントを使って調べられます。シーン失敗テストなどは意図的に例外を投げるため、例外設定で全C++例外のスロー時に中断する設定を有効にしている場合、想定どおりの例外でも止まります。

`CPU` / `GPU` のテストカテゴリも登録しています。GPUの10ケースはWARPを使ったDirect3D 12の検証です。renderer終了テストには10秒待機を意図的にタイムアウトさせるケースがあります。実機GPUの長時間負荷試験は含みません。

## 新しいテストを書く

たとえばシーン関連のテストは `tests/SceneLifecycleTests.cpp` に追加します。

1. 既存の匿名名前空間内に `void MyNewTest()` を定義します。
2. 公開APIや依存先の差し替えを使って条件を作り、`Require` で期待動作を確認します。
3. ファイル末尾の登録一覧に `TEST(MyNewTest, "new expected behavior", Cpu)` を一件追加します。一覧を継続する行の末尾には、既存行と同様に `\` が必要です。
4. `VisualStudioTests` をビルドし、新しいケースを選択して実行します。
5. 意図した条件で失敗することを確認してから本体を修正します。成功後に共通化し、関連テストを再実行します。

登録一覧はCLIとVisual Studioで共有されます。`main()`への `run(...)` 追加は不要になりました。ケース名は関数名、説明は登録一覧の文字列です。マクロで登録しているため、テストエクスプローラーのソースへの移動先はファイル末尾の `GAME_TEST_SUITE` 行になる場合があります。実際にデバッグする関数へ移動し、関数内にブレークポイントを置いてください。

## コマンドラインから同じテストを実行する

リポジトリのルートで実行します。PowerShellからスクリプトを絶対パスで呼ぶ場合は、作業ディレクトリがどこでも利用できます。

```powershell
# ビルドして、Visual Studioと同じ実行基盤で全ケースを実行
.\tools\test_visual_studio.ps1

# 検出されたケースの一覧
.\tools\test_visual_studio.ps1 -List

# 既にビルド済みのDLLから、通知失敗のテストだけを実行
.\tools\test_visual_studio.ps1 -NoBuild -Filter 'FullyQualifiedName~FailingNotificationDoesNotReplaceLoadFailure'

# 従来のAsset・RenderUpload・SkinningのGPU3ケースを選択
.\tools\test_visual_studio.ps1 -NoBuild -Filter 'FullyQualifiedName~Gpu'

# renderer終了・リサイズの8ケース（CPU1・GPU7）を選択
.\tools\test_visual_studio.ps1 -NoBuild -Filter 'FullyQualifiedName~RendererLifecycleTests'

# UIの寸法変更とクリック判定（CPU4件、RmlUiを使用）
.\tools\test_visual_studio.ps1 -NoBuild -Filter 'FullyQualifiedName~UiViewportTests'

# Release構成
.\tools\test_visual_studio.ps1 -Configuration Release

# 本体・エディター・従来のCLIテスト・ネイティブテスト・シェーダーを両構成で検証
.\tools\validate_core.ps1
```

結果のTRXファイルは `x64/<構成>/VisualStudioTests/TestResults` に保存されます。フィルターに一致するテストが0件の場合、スクリプトは成功扱いにせずエラーにします。`-NoBuild` は編集後の再ビルドを行わないため、コードを変更したら省略するか、先にVisual Studioでビルドしてください。

この環境のネイティブアダプターでは、`Category` 属性によるGUI上の分類とCLIの `TestCategory` フィルターは連動しません。CLIでは関数名やクラス名で選択します。`~Gpu`は従来の3ケースだけに一致するため、renderer終了テストには上記のクラス名フィルターを使ってください。

従来の `test_assets.ps1` などのスクリプトも利用できます。CLIではGPUケースに `-Gpu` / `--gpu` の指定が必要で、Visual Studioの「すべて実行」ではCPU・GPUの両方を実行します。

## 実装と分離

- `tests/TestSupport.h` が、同じ登録一覧をCLIの実行関数またはネイティブの `TEST_CLASS` / `TEST_METHOD` へ展開します。子プロセスで既存EXEを呼ぶ方式ではなく、テスト関数を直接実行します。
- `tests/VisualStudio/VisualStudioTests.vcxproj` は、テストソースと本体の `EngineFramework.lib` / `GameModule.lib` をリンクします。従来のCLIプロジェクトは、それぞれ必要な本体ソースをコンパイルします。
- `RendererLifecycleTests`のCLIも`EngineFramework.lib`をリンクします。終了手順・故障注入・実行環境の制限は[描画終了と資源の寿命](render-shutdown.md)を参照してください。
- `UiViewportTests`は`EngineFramework.lib`と同梱の`rmlui.lib`をリンクします。GPUなしで実際のUIレイアウトとクリックを検証します。[ウィンドウ寸法の契約](viewport-resize.md)も参照してください。
- 本体のprivateメンバの公開、テスト専用friendの追加、ゲーム側へのテストフレームワークの依存追加はしていません。
- 画像の一時ファイルはテストEXE/DLLの場所を基準に作成します。Visual Studioのテストホストのインストール先へは書き込みません。
- ネイティブテストは実行時のディレクトリを一時的にリポジトリへ切り替え、終了時に戻します。診断sink・ディレクトリ・キャッシュを共有するため、テストメソッドの実行は共通mutexで直列化しています。テスト対象が使用する非同期処理は維持されます。

Visual Studio Installerで「C++によるデスクトップ開発」とC++用テスト機能が必要です。このPCでは、ネイティブテスト用ヘッダー・ライブラリとVSTestのインストールを確認済みです。

2026-09-08の検証では、Debug / Releaseともネイティブ67件、従来のCLI全10種類、本体・エディターのビルド、依存規則、プロジェクト登録、全シェーダー検証が成功しました。ログは `x64/visual-studio-integration-validation.log` に保存しています。System32からの個別実行・GPU3件の選択実行、0件に一致するフィルターの拒否、独立した故意の失敗によるアダプターの失敗通知も確認しました。テストエクスプローラーと共通のVSTest実行基盤で検証しており、Visual Studio画面の手動操作は実施していません。

公式説明: [C++用テストフレームワーク](https://learn.microsoft.com/en-us/visualstudio/test/how-to-use-microsoft-test-framework-for-cpp?view=visualstudio)、[テストエクスプローラー](https://learn.microsoft.com/en-us/visualstudio/test/run-unit-tests-with-test-explorer?view=visualstudio)。
