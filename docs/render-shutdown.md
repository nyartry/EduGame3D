# 描画終了と資源の寿命

ゲームやツールの終了時には、シーン・モデル・UIを破棄する前に、送信済みのGPU処理を終える必要があります。C++の変数は宣言と逆順に破棄されるため、rendererを先に宣言するだけでは、ほかの資源所有者を守れません。

## アプリケーション側の順序

`Win32Application::Run`では、資源所有者の宣言後、GPU初期化の前に`RenderShutdownGuard`を置きます。正常終了、初期ロード失敗によるreturn、例外のいずれでも同じ順序になります。

```cpp
Dx12Renderer renderer;
// effects、ui、game、scenesなどの資源所有者をここで構築する。
RenderShutdownGuard shutdown(renderer);
renderer.Initialize(window, width, height);
// 初期化・更新・描画。スコープを抜けるとshutdownが先にGPUを停止する。
// その後、scenes・ui・effectsなどが破棄される。
```

アプリ全体で使う資源所有者を追加するときは、ガードより前に宣言してください。ガードより後に宣言したローカル資源は、ガードより先に破棄されます。フレーム中に破棄する資源には、既存の`IRenderResourceLifetime::DeferRelease`を使います。

エディターは`AnimationEventEditorApp::Impl`のデストラクタ本体で`Shutdown()`を呼びます。モデル・グリッド・ImGuiの資源が生きている間にGPUを停止し、その後ImGuiを終了します。コンテキストだけ生成された状態や、バックエンドの途中初期化も扱います。

SceneManagerは引き続き非同期のCPU準備を終了させてからシーンを破棄します。`Prepare()`でGPUやUIを操作しない契約は維持してください。

## rendererの終了契約

`Dx12Renderer::Shutdown() noexcept`はメインスレッドから呼ぶ終端操作で、二度呼んでも解放を繰り返しません。終了後は再初期化・描画・リサイズに使わず、必要なら新しいインスタンスを作ります。取得済みの生のcommand queueにも、終了後は命令を送信しないでください。

1. 記録中フレームを送信せず中断する。
2. キュー末尾に新しいフェンスを設定し、送信済み命令の完了を待つ。`ExecuteCommandLists`後にフレーム用`Signal`が失敗した場合も、新しいフェンスで待つ。
3. GPU完了またはデバイス削除を確認してから、記録中のcommand listを破棄する。
4. 遅延解放を一件ずつ消費する。あるコールバックが例外を投げても診断し、残りを実行する。コールバックや診断sinkから再度要求された解放も、安全確認前には実行しない。

rendererのデストラクタも同じ操作を呼びます。ただし、その時点ではほかの所有者が既に破棄され得るため、アプリ側のガード／明示的な終了処理は必要です。

`WaitForGpu()`は通常フレーム間の同期待ちです。記録中に呼ぶと例外になります。記録を中断する終端処理の`Shutdown()`を、通常の待機の代わりに使わないでください。

## 待機失敗とデバイス削除

GPU待機は一回につき10秒を上限とし、イベントの戻り値と実際のフェンス完了値を確認します。以前タイムアウトしたイベントが遅れて届いても、今回の完了とは扱いません。

通常の待機失敗は操作・原因を持つ例外として伝えます。終了処理中の待機失敗は、デバイス削除済みならその状態を確認し、まだ生きていれば任意取得した`ID3D12Device5::RemoveDevice()`で削除を試みます。削除確認後に解放し、待機失敗と終了措置を診断へ記録します。

**完了もデバイス削除も確認できない場合は、`std::terminate()`で終了します。** タイムアウトだけを根拠に未完了の資源を解放しません。この非常経路はGPUの復元ではありません。[RemoveDevice](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device5-removedevice)と[GetDeviceRemovedReason](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-getdeviceremovedreason)の契約に基づく、破棄のための措置です。

初期化途中でqueue・フェンス・イベントが揃っていない場合、現行rendererはまだ描画命令を送信していないため、待機なしで片付けます。初期化中のGPU送信を追加する場合は、この前提も更新してください。

## シーンの解放失敗

`IScene::Unload()`は通常、失敗せず完了するよう実装します。学生の拡張処理から例外が出ても、SceneManagerは原因を診断し、ほかのシーンの解放を続けます。メンバー資源はRAIIで破棄できるようにしてください。失敗した任意の外部処理を自動でやり直す契約ではありません。

診断sinkやメッセージの組み立てが失敗しても、解放時の例外が元の更新・描画エラーを置き換えないようにしています。

## 検証

`RendererLifecycleTests`は本体の`EngineFramework.lib`を使い、CLIとVisual Studioで同じテストを実行します。GPUケースは明示的なWARPアダプターを使用し、GameAppのGPU選択は変更していません。

```powershell
.\tools\validate_core.ps1
# ビルド済みのrenderer終了テストをGPUケースも含めて実行
.\x64\Debug\RendererLifecycleTests\RendererLifecycleTests.exe --gpu
# Visual Studioの実行基盤で終了テストだけを選択
.\tools\test_visual_studio.ps1 -Filter 'FullyQualifiedName~RendererLifecycleTests'
```

未初期化、途中初期化、正常終了、送信済み処理と記録中フレームの同居、破棄順、元の例外保持、解放失敗、再入、二重終了を検証します。GPUキューを止めるケースでは、10秒のタイムアウト後もデバイス削除の確認まで資源が残ることを確認します。削除API非対応環境では該当ケースがスキップを記録するため、成功件数だけで実行範囲を判断しないでください。

WARPでの故障注入は、実機GPUの物理障害や全ドライバーの動作保証とは別です。削除非対応かつ待機不能の場合のプロセス強制終了は、通常のネイティブテスト内では起こしていません。
