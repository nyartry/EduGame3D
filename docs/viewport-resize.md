# ウィンドウ寸法とシーン・UI

ゲームの描画、カメラ、UI、ポインターは、現在のクライアント領域のピクセル寸法を使います。初期値は1280×720です。Game固有の配置はGame側で決め、Frameworkは寸法通知と描画先の更新を担当します。

## 通知の順序

`Win32Application`は`WM_SIZE`を記録し、次の描画開始前に`GetClientRect`から最新寸法を取得します。`Dx12Renderer::Resize`でGPUの使用完了を待ち、描画先と深度バッファを作り直してから、`SceneManager::Resize`を呼びます。Windowsのコールバック内ではGPU操作やシーン処理を行いません。

`SceneManager`は全ての有効なシーンへ`IScene::OnResize(width, height)`を通知し、ロード表示とフェードの矩形も更新します。Additiveで重なったシーンや、フェード中に保持しているシーンも対象です。同一寸法の繰り返しと、幅または高さ0は無視します。

新しいシーンの順序は、同期・非同期とも`Prepare → OnResize → Activate`です。非同期の`Prepare`中には通知しません。準備が終わってメインスレッドへ戻った時点の最新寸法を、`Activate`の直前に一度渡します。シーンfactoryが参照するGameの初期寸法は変更しません。

```cpp
void MyScene::OnResize(std::uint32_t width, std::uint32_t height)
{
    if (width == 0 || height == 0) return;
    m_width = width;
    m_height = height;
    // 最初の通知はActivate前。資源がまだなければ寸法の保存だけ。
    if (!m_camera) return;
    m_camera->SetLens(m_fovY, float(width) / float(height), m_nearZ, m_farZ);
    // 作成済みのHUDやUIドキュメントも、ここで配置を更新する。
}
```

`OnResize`はメインスレッド専用です。`Prepare`ではカメラ投影やUIなど、現在の画面寸法に依存する有効化処理を行わないでください。候補シーンの通知が例外になった場合は、`resize`段階のロード失敗として旧シーンを保持し、再試行できます。有効なシーンへの通知失敗は起動側へ伝播し、寸法が不整合なまま描画を続けず、[終了契約](render-shutdown.md)に従って停止します。

## UIとポインター

`IUiDocument::Resize`はRmlUiのコンテキストを実際の寸法へ更新し、その場でレイアウトも更新します。直後の最初のポインター入力から新しい配置で判定します。`GetElementBounds(id)`は要素のborder boxをクライアント座標で返し、存在しない要素は`std::nullopt`です。

タイトルのボタンは`vw`／`vh`で配置し、別途SpriteBatchで描く文字はこの実際の矩形へ収めます。マウス入力も`ScreenToClient`で得た座標をそのまま渡します。GameSceneは投影のアスペクト比、画面下の画像、HUDの中央・右端配置を更新します。

## 最小化と復帰

最小化または0寸法の間は最後の正寸法を維持し、描画と固定更新を止めてWindowsの次のメッセージを待ちます。UIには入力解除を渡します。復帰やウィンドウ移動・サイズ変更の後は時計と固定更新用の入力バッファをリセットし、停止していた時間をシミュレーションの追いつき更新に含めません。

これはDPI変更やフルスクリーン切り替えの包括的な契約ではありません。新しいシーンを追加するときは、縦長・横長、最小化・復帰、非同期ロード中の変更も確認してください。ゲーム固有のHUDや画像の全てを自動で縮小する仕組みではないため、読みやすい最小寸法や配置は作品側で設計します。

## 回帰確認

- `SceneLifecycleTests`: 初回通知の順序とスレッド、全シーンへの通知、0・同一寸法、準備中の変更と最新寸法、通知失敗と再試行。
- `UiViewportTests`: 実際のRmlUiで寸法変更直後の要素矩形とクリック判定、0寸法の維持。
- `RendererLifecycleTests`: WARPで縦横のサイズ変更後に複数フレームを送信し、0寸法の無視と記録中の変更拒否を確認。

実行方法は[Visual Studioテスト](visual-studio-tests.md)を参照してください。
