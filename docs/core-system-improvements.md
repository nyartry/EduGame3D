# コアシステム監査に基づく改善

実装日: 2026-09-07。対象は `docs/core-system-audit.md`。既存の移動操作、水平ルートモーション合成、ジャンプ開始から着地までのプログラムY優先を維持して共通基盤を拡張しました。

## 実装内容

| 監査項目 | 変更と契約 |
| --- | --- |
| 時間・入力 | `FixedStepClock` をLauncherへ接続。1/60秒、最大8更新、経過時間は最大0.25秒を受理し、超過した整数ステップを捨てます。フォーカス離脱・復帰では蓄積時間と保留入力をリセットします。入力の押下・解放は固定更新まで保持し、一度の更新だけで公開します。同じ更新の複数の読み手は同じ状態を読みます。UI・フェード・HUDは `UpdateFrame` で描画フレームごとに更新します。 |
| ルートモーション | `AnimationPlayback` と `RootMotionExtractor` へ抽出を分離。一周ちょうど・複数周・小数クリップ長の境界を処理し、ポーズとイベントも同じ局所時刻・区間を参照します。 |
| GPUバッファ | 定数バッファを容量超過時にページ追加する方式へ変更。各ページを送信フェンスに関連付け、完了後だけ再利用します。SpriteとCPUスキニングの動的頂点もUpdateではCPUデータを保持し、Draw時にフェンス管理された領域へコピーします。記録中の遅延廃棄も、そのフレームの送信完了まで待ちます。 |
| 衝突・接地 | `ICollisionSurface`・`ICollisionQuery`・`CollisionWorld` をFrameworkへ追加。GameSceneの床・側面の登録と押し戻し、CharacterGroundingとPrimitiveObjectの床問い合わせを接続。除去・自己除外・一方向床の契約を定義しました。既存のSetGround等も抽象インターフェースで利用できます。 |
| ロード・共有 | `Actor::Prepare` と各モデルのPrepare/Activateを分離。GameSceneはワーカーでFBX・追加アニメーション・RGBA画像を準備し、メインスレッドでGPU資源化します。準備中の同一モデル・アニメーションは `ModelAssetCache` で共有。画像はスレッド間の弱参照キャッシュ、GPUテクスチャ・マテリアルはdevice・パス・sRGB/用途・既定色を考慮して共有します。旧シーンは準備とActivateの成功まで保持し、失敗時は戻って再試行できます。 |
| AABB・法線 | 有限点・空状態を定義した `Aabb` と足元原点/高さ調整の `ModelFit` を共有。ボーン法線は逆転置、接線は通常の行列を使い、重み合成後に正規化します。特異行列・無効重み・方向相殺時の規則をCPU/GPUで揃えました。 |
| アニメーションイベント | ランタイムとエディターで型・schema検証付きJSON・区間発火を共有。隣接 `.anim_events.json` を読み込み、GameSceneが各固定更新で音・エフェクトのcueを既存サービスへ渡します。詳細は `docs/animation-playback.md`。 |
| カメラ・パス・診断 | FollowCameraをCameraBaseへ移行して初期姿勢・追従を維持。モデルのテクスチャ探索もUTF-8変換規約へ統一。HRESULT・呼出元・操作名・画像パスを診断へ記録し、出力先を交換可能にしました。 |
| 検証 | プロジェクト/filtersを変更しない `sync_vs_filters.ps1 -Check` を追加し、EngineFrameworkのビルド前に実行。Windows PowerShellとPowerShell 7で同じ順序を生成します。`validate_core.ps1` は依存規則、登録、Debug/Release本体・エディター、回帰、実GPU、全HLSLのVS/PSを一括検証します。 |

## 検証コマンド

リポジトリで次を実行します。Visual Studio C++ビルドツール、Windows SDK、同梱のthird_party依存が必要です。

```powershell
.\tools\validate_core.ps1
```

Debugだけなら `-Configurations Debug`、Releaseだけなら `-Configurations Release` を指定します。GPU回帰はD3D12 WARPを使用します。

| テスト | 主に確認すること |
| --- | --- |
| MathCoreTests | SRT、角度、法線、有限値、平滑化の既存回帰 |
| TimeInputTests | 30/60/144FPSで更新数・移動距離一致、長時間停止、0回/複数回更新、短いタップと複数読者 |
| RootMotionTests | 周回境界、ジャンプY、イベント境界、schema/JSON、日本語パス、エディター相互保存 |
| RenderUploadTests | 容量超過、未完了フェンスの非重複、1025描画×3フレームの定数・頂点計6150件をGPUから読み戻し |
| PhysicsTests | 登録/除去、床・側面・境界、自己除外、FPS/0.5秒停止条件のジャンプ、AABB/ModelFit |
| ModelPreparationTests | 実FBXのworker準備、準備後にテスト用FBXを削除してActivate、キャッシュの共有/解放、失敗後の再試行 |
| AssetTests | 日本語パス、WICのRGBA変換、worker→mainの画像共有、診断、sRGB/linear区別、GPUテクスチャ・マテリアルの共有 |
| SceneLifecycleTests | 同期/非同期のfactory・Prepare・Activate失敗、旧シーン保持、GPU完了後Unload、worker/mainの担当、UIと固定更新の分離 |
| SkinningTests | 非一様スケール・重み合成・特異行列・無効影響等8条件でCPUと実VSの位置/法線/接線を比較 |

検証結果: Debug/Releaseとも、本体4プロジェクト、9種類の回帰テスト、WARP実GPUテスト、4シェーダー×VS/PSの検証に成功しました。結果は `x64/core-validation.log` に保存しています。登録漏れを一時的に作る確認では、Checkが不一致を検出し、全プロジェクト/filtersのハッシュを変えないことも確認しました。

## 今回の範囲と残る検討

- 固定更新の描画補間はまだ使っていません。停止後は実時間全量を追いかけず、シミュレーションを一定幅で再開する方針です。
- 非同期遷移は同時に1件までとし、旧シーン一組と準備中の新シーン一組を保持します。メモリのバイト上限やピーク量は未計測です。Prepare/Activate途中の失敗資源はRAIIで破棄される必要があります。
- テクスチャ転送自体のバッチ化・分割転送は未実施です。重複した読込・GPU生成を削減しましたが、各転送の同期待ちは残っています。画像デコード数とアップロード数は `ImageLoader::GetDecodeCount` / `Texture2D::GetUploadCount` で確認できます。
- CPUモデルの共有は準備中のimportデータが対象です。実行中のスケルトン・クリップ・変形頂点を完全に共有する設計までは変更していません。
- `HitboxStart` / `HitboxEnd` / `Custom` のゲーム固有動作や新規の足音素材は追加していません。音・エフェクトのcueはGameで登録されたIDを使います。
- ゲーム画面の手動操作、実機GPUでの長時間負荷試験、ロード時間やFPSの改善量の計測は未実施です。
