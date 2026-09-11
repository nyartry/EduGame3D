# 学生配布用3Dゲームひな型 — コアシステム調査・引き継ぎ

作成日: 2026-09-09（日本時間）

この資料は、別のPCのCodexが、会話履歴なしでコアシステムの調査・強化を引き継ぐための資料です。既存実装の説明、コードから判断したリスク、追加機能の提案を区別して記載しています。今回の作業は資料化であり、以下の改善案は未実装です。

追記（2026-09-11）: 上記および第1〜10節は作成時点の記録です。改善価値を再調査した結果は[改善価値と実施単位](student-template-improvement-review.md)、引き継ぎ後の変更状況は第11節を参照してください。コミットはユーザーが行い、作業は1コミット分ずつ区切ります。

## 1. 最優先で引き継ぐユーザーの目的

ユーザーから明示された前提（要旨）は次のとおりです。

> このプロジェクト自体のゲーム完成は目的ではない。学生配布用のひな型であり、学生はこれを使って就職作品を制作する。コアシステムを充実させたい。ジャンルを固定する要素は入れたくない。ただし、3Dゲームという範囲は固定でよい。

評価基準は「学生が異なる3D作品を制作するとき、共通機能を利用・理解・拡張できるか」です。ひな型に勝敗やゲームの一周があるかどうかではありません。

初回の調査回答では、目的を完成ゲームと誤解し、敵AI・HP・ダメージ・勝敗・リザルト・リトライ・`GameSession`を優先提案しました。この評価は訂正済みです。これらを必須コア要件として引き継がないでください。

コアと作品側の責任分担の例:

| コアが提供する仕組み | 学生の作品側が決める内容 |
| --- | --- |
| 物体の接触・領域への出入りを通知する | ダメージ、取得、スイッチ作動などの意味 |
| アニメーションの再生・合成・イベントを扱う | 動作名、攻撃中の行動制限、コンボ規則 |
| 入力を任意のアクションや数値へ変換する | 操作の意味、キー割当の初期値 |
| シーンの切替、更新停止、重ね合わせを扱う | タイトル、リザルト、メニュー等の構成 |
| 設定やデータを読み書きする | 保存項目、進行状況、スコアの形式 |

現在の移動・攻撃・HUDなどは、サンプルとして存在すること自体は問題ではありません。サンプルを削除することも今回の要件ではありません。作品固有のルールをFrameworkへ必須機能として移さないことが重要です。

## 2. 調査の基準と状態

| 項目 | 内容 |
| --- | --- |
| リポジトリ | `3D-game-for-open-campus-Koyo` |
| 調査基準コミット | `da386cc30bfff82a77cb77070afebf18fb9c59fd`（`テストを実装`） |
| 調査時のブランチ | `develop` |
| 基準コミット時の作業ツリー | 変更なし。資料追加前にも確認済み |
| 実行環境 | Windows、PowerShell、Visual Studio C++ビルド／テスト環境、Windows SDK、x64 |
| 資料化時の変更 | この資料とREADMEの案内のみ。C++・HLSL・プロジェクト設定は変更なし |

別PCのHEADが異なる場合は、現在のソースを優先してください。行番号は調査時の目安です。相対リンクとシンボル名から実装を探せるようにしています。以前の資料にあるPC固有の絶対パスへ移動する必要はありません。

判定は「制作開始の土台は整っている。汎用コアとしては、オブジェクト管理・座標階層・3D衝突問い合わせ・入力に補強の価値が高い」です。機能不足として挙げる項目は、現在の小規模サンプルが壊れているという意味ではありません。

## 3. 既存の構造と維持する契約

- `src/Framework`: 共通API、ランタイム、各バックエンドの実装。
- `src/Game`: サンプルのシーン、操作、モデル指定、作品固有の規則。
- `src/Launcher`: Win32起動と、Game・描画・音・UI・エフェクトの組み立て。
- `tools/AnimationEventEditor`: 同じEngineFrameworkを使うアニメーションイベント編集ツール。
- `tests`: コア回帰テストとVisual Studioネイティブテスト。

詳細は [architecture.md](architecture.md) を参照してください。強化時も次を維持します。

1. FrameworkからGameへ依存しない。GameにWin32、DX12、Assimp、RmlUi、Effekseerの具象型を漏らさない。
2. 静的ライブラリの境界と、学生がソースを読んで変更できる構成を維持する。DLL化や別エンジンへの移行は本調査の提案ではない。
3. `Prepare()`は非同期ロード時にワーカーで行うCPU処理、`Activate()`はメインスレッドで行うGPU／UI処理。部分失敗時の資源はRAIIで破棄可能にする。
4. 固定更新とフレーム更新を分離し、入力の押下・解放を固定更新まで保持する契約を壊さない。
5. 描画中／GPU処理中の資源寿命は、既存のフェンスと遅延解放の仕組みに接続する。
6. Core/Mathはシーン・描画・Win32に依存しない。DirectXMathの値型は現時点で意図的な共通依存である。
7. 既存サンプルのルートモーションとジャンプの契約を維持する。特に既定の水平合成とプログラム側のY制御を参照する。

## 4. 確認済みの実装

| 分野 | 実装されていること | 主な参照先 |
| --- | --- | --- |
| 時間・入力 | 60Hz固定更新、最大8更新、経過時間上限0.25秒、フォーカス変化でリセット、入力エッジの保持 | [FixedStepClock.h](../src/Framework/Core/Time/FixedStepClock.h)、[SimulationInputBuffer.h](../src/Framework/Scene/Input/SimulationInputBuffer.h)、[Win32Application.cpp](../src/Launcher/Win32/Win32Application.cpp) の `Run` |
| シーン | 同期／非同期ロード、Single／Additive、フェード、失敗時の旧シーン維持・再試行、通常遷移での遅延破棄 | [SceneManager.cpp](../src/Framework/Scene/Core/SceneManager.cpp) の `LoadScene`、`CommitLoadedScene`、`RecoverLoadFailure` |
| GPU資源 | フェンスに対応したアップロード領域、容量超過時のページ追加、遅延解放 | [FrameUploadBuffer.h](../src/Framework/Rendering/Core/FrameUploadBuffer.h)、[FenceRetiredPagePool.h](../src/Framework/Rendering/Core/FenceRetiredPagePool.h)、[IRenderResourceLifetime.h](../src/Framework/Rendering/Core/IRenderResourceLifetime.h) |
| アセット | パス解決、CPU側モデル準備、画像キャッシュ、テクスチャ／マテリアル共有、画像失敗時の代替と再試行 | [AssetPathResolver.cpp](../src/Framework/Assets/AssetPathResolver.cpp)、[ImageLoader.cpp](../src/Framework/Assets/ImageLoader.cpp)、[ModelAssetCache.cpp](../src/Framework/Models/ModelAssetCache.cpp) |
| モデル・アニメーション | 静的／スキンモデル、CPU／GPUスキニング、単発／ループ再生、ルート変位抽出、イベントJSONと編集ツール | [SkinnedModel.h](../src/Framework/Models/SkinnedModel.h)、[AnimationPlayback.h](../src/Framework/Animation/AnimationPlayback.h)、[AnimationEvents.h](../src/Framework/Animation/AnimationEvents.h) |
| 数学 | SRT変換、正規化、角度、法線行列、AABB、モデル高さ・原点調整 | [Transform.h](../src/Framework/Core/Math/Transform.h)、[MathUtils.h](../src/Framework/Core/Math/MathUtils.h)、[Aabb.h](../src/Framework/Core/Math/Aabb.h)、[ModelFit.h](../src/Framework/Models/ModelFit.h) |
| 衝突 | 床問い合わせ、境界押し戻し、物体同士の高さ＋XZ半径による押し戻し、登録／解除 | [ICollisionQuery.h](../src/Framework/Physics/ICollisionQuery.h)、[CollisionWorld.cpp](../src/Framework/Physics/CollisionWorld.cpp)、[CollisionBody.cpp](../src/Framework/Gameplay/CollisionBody.cpp) |
| カメラ | Follow、SpringFollow、FirstPerson、Orbit、Spline | [CameraController.h](../src/Framework/Scene/Cameras/CameraController.h) と同ディレクトリ |
| 音・演出・UI | BGM／SEと音量API、Effekseer再生、Sprite、RmlUi文書・クリック・クラス設定 | [IAudioService.h](../src/Framework/Audio/IAudioService.h)、[IEffectService.h](../src/Framework/Effects/IEffectService.h)、[IUiService.h](../src/Framework/UI/IUiService.h) |
| 診断・検証 | HRESULT／ソース位置を含む例外、交換可能な診断sink、回帰テスト、依存・登録チェック | [Diagnostics.h](../src/Framework/Core/Diagnostics/Diagnostics.h)、[ExceptionUtils.h](../src/Framework/Core/Diagnostics/ExceptionUtils.h)、[validate_core.ps1](../tools/validate_core.ps1) |

アセット共有は万能ではありません。モデルのimportデータ共有と、実行中のモデル・スケルトン・頂点データの完全共有を混同しないでください。GPUテクスチャの個別転送には同期待ちが残ります。詳しくは [core-system-improvements.md](core-system-improvements.md) を参照してください。

## 5. 強化前に扱う安定性・配布上の課題

この節は静的コード調査で確認した実装と、そこから判断したリスクです。ゲーム画面での不具合再現や障害注入は未実施です。修正時は、対象の失敗条件と期待動作を先に具体化してください。

### R1. 初期シーンのロード失敗を起動側で扱う

- 事実: [Win32Application.cpp](../src/Launcher/Win32/Win32Application.cpp) の `Run`（112行付近）は、初期 `LoadScene()` のbool戻り値を確認していない。
- リスク: シーンロード側で例外を処理してfalseを返すと、初期シーンが空のままメインループへ進み得る。
- 最小方針: 初期シーンがない場合の失敗を明示し、診断と終了、または独立したエラー表示へつなぐ。通常のシーン切替失敗時には既存の旧シーン維持を続ける。
- 完了条件: 初期ロード失敗を注入して、無表示のまま継続しないこと、原因が分かること、通常の再試行が壊れないことを確認する。

### R2. 例外時を含む終了順とGPU資源寿命を揃える

- 事実: `Run`のローカル変数ではrendererより後にscenesを生成し、正常終了時のみ末尾で `WaitForGpu()` を呼ぶ。[SceneManager.cpp](../src/Framework/Scene/Core/SceneManager.cpp) のデストラクタはactive sceneを即時 `Unload()`・破棄する。
- リスク: 途中例外ではscenesがrendererより先に破棄され、送信済みGPU処理の完了前にシーン資源が解放される可能性がある。通常遷移の遅延解放とは別の経路である。
- 関連事実: [Dx12Renderer.cpp](../src/Framework/Rendering/Core/Dx12Renderer.cpp) のデストラクタ（78行付近）は例外を投げ得るGPU待機を呼び、`FlushGpu`／`MoveToNextFrame`は無期限待機を行う。
- 最小方針: 通常終了、途中例外、初期化途中、記録中フレームの中断を区別し、送信済み資源を安全に解放する終了契約を作る。記録中に単に `WaitForGpu()` を追加すると既存契約に反して例外になるため注意する。
- 完了条件: 破棄順、未完了フェンス、途中初期化・例外のテストを追加し、デストラクタから二次例外が漏れないことを確認する。GPUデバイス復元の全面実装は、この最小修正とは別の候補である。

### R3. ウィンドウ寸法変更の契約を接続する

- 事実: ウィンドウは `WS_OVERLAPPEDWINDOW` だが、[Win32Application.cpp](../src/Launcher/Win32/Win32Application.cpp) の `WindowProc`は `WM_SIZE` を扱わない。[Dx12Renderer.cpp](../src/Framework/Rendering/Core/Dx12Renderer.cpp) には `Resize()` が既にある。
- 不足: カメラ投影、UI、ポインター座標の扱いを含めたサイズ通知が接続されていない。
- 方針: 汎用ひな型の拡張としては一貫したサイズ変更対応が有用。固定サイズ化は短期の代替案であり、ユーザーが確定した仕様ではない。
- 完了条件: リサイズ・最大化・最小化・復帰後に描画比率とUI入力が一致し、幅／高さ0で資源を不正生成しない。

### R4. GameApp単独で成立する配布・起動手順を用意する

- 事実: [GameApp.vcxproj](../GameApp.vcxproj) のPostBuildはAssimp DLLのコピーのみ。[BasicColorPipeline.cpp](../src/Framework/Rendering/Pipelines/BasicColorPipeline.cpp) 等は相対パスのHLSLを実行時コンパイルする。
- 関連事実: [AnimationEventEditor.vcxproj](../AnimationEventEditor.vcxproj) は共通出力先へHLSLをコピーする。全ソリューションの成功だけでは、GameApp単独の成果物が自己完結していると証明できない。
- リスク: ContentやHLSLを含めず別フォルダーへ移した場合の起動が保証されていない。`AssetPathResolver`の上位ディレクトリ探索により、開発環境では不足が見えにくい。
- 完了条件: EXE、必要DLL、Content、採用した方式のシェーダーを一括収集できる。ソースツリー外の配布フォルダーと異なる作業ディレクトリから、実際のモデル読込まで確認する。

### R5. 学生が障害を追える診断を整える

- 事実: [Win32Application.cpp](../src/Launcher/Win32/Win32Application.cpp) の診断sinkは `OutputDebugString`。音声やモデルの一部は共通sinkを経由せず出力する。音声・UI初期化のbool戻り値も起動側で確認していない。
- 方針: ファイルログと出力の統一、起動時に必須／任意のサービスが失敗した場合の扱いを整える。音声失敗を必ず致命的エラーにする、などの固定方針は避ける。
- 完了条件: デバッガなしでも操作名・資源パス・原因が追える。既存テストが守っているログの重複防止と、sink例外によって元の失敗を失わない契約を維持する。

## 6. ジャンルを固定しないコア拡張案

以下は推奨案です。API名・配置・順序の細部がユーザー承認済みという意味ではありません。着手時に現在の実装との差を再確認し、一つの機能を利用例・検証まで通す単位で進めます。

### C1. オブジェクトの所有・生成・削除・参照管理（優先: 高）

現状: [Actor.h](../src/Framework/Gameplay/Actor.h) はライフサイクル関数のインターフェースで、所有と追加は [GameScene.h](../src/Game/Scenes/GameScene.h) のprivateな `AddActor()` にある。更新はvectorの直接走査。衝突登録は非所有ポインターを保持し、破棄前の解除は呼び出し側の責任。

最初の実装候補:

- 小さなシーン所有のActor管理層に、追加・削除予約と反映タイミングをまとめる。
- 破棄済み参照を検出できるID／Handle、更新の有効／無効、描画の表示／非表示を用意する。
- Actorの解除を衝突・イベント購読・GPU資源寿命と連携させる。CPUの削除予約だけではGPU安全性は完結しない。
- まず既存のActor派生を利用する。ECS化や全面的な継承構造の変更を前提にしない。

完了条件: 更新中の自己削除・別Actor削除・生成、二重削除要求、古いHandle、シーン破棄を安全に処理できる。反映前後の更新対象が明確で、衝突登録が残らない。任意の立方体を出し入れする小さな利用例を用意する。

### C2. Transform階層とローカル／ワールド座標（優先: 高）

現状: [Transform.h](../src/Framework/Core/Math/Transform.h) は単体のSRT値型で、シーン階層から独立している。ボーン階層はモデル内にあるが、一般Actorの親子管理APIはない。

最初の実装候補:

- Core/Mathの値型は維持し、Scene側に親子管理とワールド行列の合成を追加する。
- 親変更時にローカル座標／ワールド座標のどちらを維持するかを明示する。
- 循環参照、親の削除、無効Handle、特異行列の扱いを決める。
- 回転と非一様スケールを合成すると、結果をSRTだけで表せない場合がある。ワールド行列の保持または制約を明文化し、無条件の再分解で情報を失わない。

完了条件: 多段親子での位置・回転・拡縮が正しく反映され、循環を拒否し、親削除・親変更の挙動が定義されている。親子の立方体や可動部品で利用例を示す。

### C3. 汎用3D衝突・空間問い合わせ（優先: 高）

現状: [ICollisionQuery.h](../src/Framework/Physics/ICollisionQuery.h) は床と境界の問い合わせ。[CollisionBody.cpp](../src/Framework/Gameplay/CollisionBody.cpp) は高さの重なりとXZ半径から押し戻す。現在の床上移動には使えるが、任意方向の3D問い合わせには不足する。

段階的な実装候補:

1. 球・AABB等の少数形状と、Raycast／Overlap、距離・位置・法線・対象Handleを持つ結果を作る。
2. レイヤーマスク、自己除外、判定のみのTriggerとEnter／Stay／Exit通知を加える。
3. 利用例に合わせてカプセル、Sweep、カメラ壁回避、必要な地形問い合わせへ広げる。

接地や移動規則は問い合わせを利用する側に置く。接触とダメージ処理を結び付けない。全形状ペア、本格的な剛体物理、空間分割の最適化を最初から一括実装する必要はない。

完了条件: 境界接触、原点が形状内、無効入力、マスク・自己除外、接触相手の削除を検証する。通知の二重発火を防ぎ、既存の床・ジャンプの回帰を維持する。マウス選択や配置判定などの利用例を付ける。

### C4. 学生が定義できる入力アクションとデバイス対応（優先: 高）

現状: [Input.h](../src/Framework/Scene/Input/Input.h) は固定列挙のキーと左クリック中心。[GameActions.cpp](../src/Game/Input/GameActions.cpp) はコード内の固定割当で、任意のアクション割当・アナログ入力・ゲームパッドの共通機構はない。

最初の実装候補: アクション名と割当を作品側で定義し、共通側はボタン・1軸・2軸の値、複数バインド、入力コンテキストを扱う。マウス移動量・ホイール・ゲームパッドを必要な単位で追加する。デッドゾーンや接続解除の規則を定義する。

完了条件: 既存の固定更新用入力バッファを維持し、押下の消失・重複がない。再割当、フォーカス喪失、デバイス切断、UIへの入力優先を検証する。FrameworkにAttack等のゲーム固有アクションを固定しない。

### C5. シーンと時間の制御（優先: 中）

現状: 非同期ロードは一件まで。Additiveは配列へ追加し、全シーンを更新・描画、最後のシーンのカメラを使用する。個別除去、下層の更新停止・入力遮断の共通方針がない。[IScene.h](../src/Framework/Scene/Core/IScene.h) の遷移要求は文字列等の複数関数で表され、Scene側からの要求はSingleになる。

実装候補: 一度だけ受理・消費する明示的なシーン操作、再読込、個別除去、レイヤーごとの更新・入力・描画方針。時間はUI等の実時間とシミュレーション時間を分け、停止・時間倍率・必要ならタイマーを追加する。完成ゲーム用の状態名や結果画面をコアへ固定しない。

完了条件: 下層を停止してもUIが動き、再開時に余分な追いつきや入力再発火がない。シーン除去後に操作対象が残らず、カメラの選択が明確。ロード失敗時の旧状態保持も維持する。ロードのキャンセルは必要性を評価する別の候補。

### C6. アニメーションのクロスフェード（優先: 中）

現状: [SkinnedModel.h](../src/Framework/Models/SkinnedModel.h) は単一の再生状態と現在クリップを持ち、`AnimationPlayOptions`はmodeとrestartを指定する。クリップ間クロスフェードは未実装。ルートモーションのBlendやスキニングの重み合成は、クリップ間のポーズ合成とは別である。

最初の実装候補: 二つのクリップ間の時間指定クロスフェード。イベントの発火元とルート変位の合成規則を、ポーズ合成と合わせて定義する。

完了条件: 遷移時間0、遷移途中の再要求、片側の終了、イベント重複、ルート変位を検証する。既存の単発／周回境界・コンボの回帰を壊さない。ブレンドツリーや上半身レイヤーは追加候補とし、最初の必須範囲にしない。

### C7. ライト・マテリアルなどの描画設定（優先: 中）

現状: [Textured.hlsl](../src/Framework/Rendering/Shaders/Textured.hlsl) の58行付近と [SkinnedTextured.hlsl](../src/Framework/Rendering/Shaders/SkinnedTextured.hlsl) で、光源方向と照明係数が固定されている。

最初の実装候補: 既存の照明を方向・色・強度・環境光の設定としてGameから渡せるようにする。描画バックエンド型を公開せず、静的／スキンモデルで同じ設定を使う。必要に応じてマテリアル設定を拡張する。

完了条件: 既定値で従来表示を維持し、Game側の設定変更が両モデルへ反映される。シェーダー検証と実機の視覚確認を行う。影、PBR、ポストエフェクト、カリングは別途必要性と負荷を見て選ぶ。

## 7. 学生向け利用性と、後から選べる候補

各機能には、短い利用例、呼び出し順、所有権、失敗時の動作、拡張箇所を説明する資料を付ける。APIが存在するだけでは配布用としての完了としない。

- 最小シーン、Actor生成・削除、親子座標、Raycastなどを個別に試せる小さなサンプル。
- コライダー、座標軸、Ray、法線、Actor数、フレーム時間などのデバッグ表示。
- 作品固有の値を変更しやすくする設定ファイル、配置データ、汎用の読み書き。保存するゲーム進行の仕様は作品側で定義する。
- アセット読込時間・メモリ・描画負荷の計測。実測後に転送のバッチ化、共有範囲の拡張、カリング等を判断する。
- 高リフレッシュレート向けの描画補間。`GetInterpolationAlpha()`はあるが、現在の描画は利用していない。

上記はすべてを一度に作る指示ではない。大規模なECS、自作剛体エンジン、汎用イベントバス、反射・スクリプト基盤、ネットワーク、大型レベルエディターは現時点の必須要件ではない。教材として学生が追える規模を保つ、という設計上の推奨である。

既存サンプルでは、HUDのHPは固定幅、SPは時間で変化する表示見本で、TitleのEXITはクリック処理未接続。この事実はデモの表示・操作の整理には役立つが、HPシステムや完成したタイトルメニューを共通コアへ実装すべき根拠にはならない。

## 8. 検証結果と限界

2026-09-09、基準コミットに対してこの会話の調査中に、`validate_core.ps1`をDebugとReleaseでそれぞれ実行し、両方とも成功した。Debugは調査担当エージェントの完了報告、Releaseはメイン側の実行出力と終了コード0で確認した。

| 検証 | Debug | Release |
| --- | --- | --- |
| 依存規則・プロジェクト／filters登録 | 成功 | 成功 |
| GameApp・AnimationEventEditor・依存ライブラリのビルド | 成功 | 成功 |
| コンソール回帰テスト10種類 | 成功 | 成功 |
| Asset・RenderUpload・SkinningのWARPテスト | 成功 | 成功 |
| Visual Studioネイティブテスト | 67件中67件成功 | 67件中67件成功 |
| HLSL 4ファイルのVS／PS、計8コンパイル | 成功 | 成功 |

資料化時にも、両構成のTRXの `total=67 / passed=67 / failed=0` を確認した。結果は調査PCの `x64/<構成>/VisualStudioTests/TestResults/` にある。`x64/`はGit管理対象外のため、別PCに結果ファイルがあるとは限らない。統合検証の成功記録は当時の実行出力・担当エージェントの完了報告に基づき、この資料に完全ログは同梱していない。TRX単体が証明するのはネイティブテストの結果である。

DebugのGameAppを非表示で8秒間起動し、プロセスが終了していないことも確認した。最後は起動した検証用プロセスを停止したため、正常終了の成功を示す検証ではない。画面・音声・実入力・ゲームシーンへの遷移が正常であることも、この起動確認だけでは証明しない。

未検証:

- ゲーム画面を実際に操作する通し確認と、Visual StudioのGUI操作。
- 実機GPUでの長時間負荷、リサイズ・最小化・復帰の実操作、GPU障害注入。
- ロード時間、メモリピーク、FPS、フレーム時間の実測評価。
- GameApp単独の配布物を、ソースツリー外・別PCで起動する確認。
- この資料のR1〜R5に対する再現テスト、およびC1〜C7の未実装機能の検証。

WARPはソフトウェアのD3D12実行環境であり、実機GPUでのゲーム全体の動作確認とは区別する。既存テストの成功を、未実装APIやすべての障害経路の保証として扱わない。資料化の作業では本体の再ビルド・全テスト再実行はしていない。

## 9. 別PCでの再開手順

1. この資料を含むリポジトリを用意する。この資料単体にはソース・Content・third_partyは含まれない。
2. 現在のHEADと未コミット変更を確認する。基準コミットへ強制的に戻さず、差があれば関連部分を読み直す。
3. リポジトリ内に追加の作業指示があれば確認する。次の関連資料を読む。
4. Visual Studio 2022のC++デスクトップ開発、v143、Windows SDK、ネイティブC++テスト機能、同梱依存・素材を確認する。環境不足とコード不具合を区別する。
5. ベースライン検証を行い、対象の課題を小さい単位で再確認・実装する。

関連資料:

- [architecture.md](architecture.md): 依存方向、サービス契約、ソース登録。
- [core-math.md](core-math.md): 座標・SRT・数値契約。
- [root-motion.md](root-motion.md)、[animation-playback.md](animation-playback.md): 再生・移動・イベント契約。
- [error-handling.md](error-handling.md): 復旧責任、診断、故障注入テストの方針。
- [visual-studio-tests.md](visual-studio-tests.md): テスト環境と個別実行。
- [core-system-improvements.md](core-system-improvements.md): 2026-09-07の改善と残る制限。
- [core-system-audit.md](core-system-audit.md): 改善前の歴史的な監査。記載の問題は多数修正済みなので、そのまま未実装リストにしない。
- [AnimationEventEditor README](../tools/AnimationEventEditor/README.md): 編集ツール。現行のファイル名規約ではFBX／イベントJSON名は表示可能ASCII、フォルダー名はUnicode可。日本語パス対応とファイル名規約を混同しない。

リポジトリルートのPowerShellで実行:

```powershell
git status --short
git rev-parse HEAD

# 読み取り専用の構造チェック
powershell -NoProfile -ExecutionPolicy Bypass -File tools/check_architecture.ps1 -RepositoryRoot .
powershell -NoProfile -ExecutionPolicy Bypass -File tools/sync_vs_filters.ps1 -Check

# ベースライン: 本体、CLI、ネイティブ、WARP、シェーダーを検証
powershell -NoProfile -ExecutionPolicy Bypass -File tools/validate_core.ps1 -Configurations Debug
powershell -NoProfile -ExecutionPolicy Bypass -File tools/validate_core.ps1 -Configurations Release
```

`check_architecture.ps1`単独実行には `-RepositoryRoot` が必要。ビルド成果物は `x64/` 以下に生成される。ゲームとエディターが同じ出力先へDLLをコピーするため、同じ構成のビルドを重複起動しない。統合検証スクリプトはこのコピーを直列化している。

ファイルを追加・移動・削除した場合は、次で `.vcxproj` と `.filters` の登録を更新し、差分を確認する。

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/sync_vs_filters.ps1
git diff --check
git diff --stat
```

この登録更新コマンドは書き込みを行う。`-Check`版とは用途が異なる。変更範囲に応じたテストを途中で実行し、影響する本体・エディターを確認する。公開APIや広い範囲のコアを変更した段階では、両構成の統合検証を行う。

テスト項目数は将来増減し得るため、67件を恒久的な期待値として固定しない。別PCでは検出された項目と未実行項目を記録する。

## 10. 推奨する着手順と次のCodexへの依頼文

着手順の推奨は、R1・R2・R3の安定性確認／修正、次にC1 → C2 → C3 → C4。その後C5〜C7を用途に合わせて進める。R4の配布・R5の診断・利用例は、学生配布前に整える。これは小さい機能単位へ分けるための目安であり、一括の全面改修計画ではない。

別PCのCodexへ渡す開始用の依頼文:

```text
docs/student-template-core-handoff.mdを読み、現在のコードとの差を確認して作業を引き継いでください。

このプロジェクトは、学生が就職作品を作るための3Dゲームひな型です。
ひな型自体のゲーム完成は目的ではなく、3D以外のジャンルやゲームルールを固定しないでください。
敵AI、HP、勝敗、リザルト等を必須コアへ追加する方針は採用していません。

まずHEAD・作業ツリー・関連資料・ビルド環境を確認し、現在のベースラインを検証してください。
最初の実装対象は、資料のR1「初期ロード失敗」とR2「例外時を含む終了順・GPU資源寿命」です。
既に修正済みなら重複修正せず、根拠を報告してください。
未対応なら失敗条件と期待動作を具体化し、関連テスト、最小限の修正、本体・エディターの検証まで実施してください。

既存のGame／Framework分離、CPU準備とGPU有効化、固定更新と入力バッファ、
ルートモーション、フェンスによる資源管理の契約を維持してください。
ECS化や全面再設計を前提にしないでください。

変更したこと、テスト結果、未検証部分、次の小さな実装候補を報告し、
この引き継ぎ資料に完了した項目と検証対象コミットを追記してください。
```

この依頼文を送るとR1・R2の実装依頼になります。今回の資料化では、この依頼を別PCや別タスクへ送信していません。

## 11. 引き継ぎ後の記録欄

各作業の完了時は、該当ID、変更内容、検証コミット、実行した検証、残る制限を追記する。未実装提案を完了済みへ読み替えない。

| 日付 | 対象 | 変更・確認内容 | 検証と未確認事項 |
| --- | --- | --- | --- |
| 2026-09-09 | 調査・資料化 | ユーザーの目的を訂正し、既存実装・R1〜R5・C1〜C7を整理。実装変更なし | 基準コミットのDebug／Release統合検証成功。限界は第8節参照 |
| 2026-09-11 | 改善価値の再調査 | `8275e850d89d344e6ff99b18fe812dc12b15d052`のコードと公式採用提出要件を照合。[評価資料](student-template-improvement-review.md)に各項目の価値・費用・実施単位を記録 | 変更前Debug統合検証成功（ネイティブ67件）。機能数の増加自体が就職に有利とは判断しない |
| 2026-09-11 | R1 初期シーンのロード失敗 | 初期`LoadScene`がfalseなら、既存の原因をダイアログ表示してループ前に終了コード1。診断の二重記録なし。初回失敗5条件の回帰テストを追加 | 検証対象は`8275e85`上の今回の作業ツリー（コミットはユーザーが行う）。Debug／Release統合検証成功、ネイティブ各68件。Debug実起動で失敗表示・終了1と通常タイトル表示・終了0を確認。詳細は以下 |

### R1の検証記録

- 変更前: `tools/validate_core.ps1 -Configurations Debug`成功。ログは`x64/student-template-baseline-Debug.log`。
- 故障注入: `Game::GetInitialSceneName()`が未登録名`R1_MissingInitialScene`を返すよう、検証用ビルドだけ変更。修正前は5秒以内に終了しないことを確認して検証プロセスを停止した。これは正常終了の成功を示すものではない。
- 修正後の故障注入: 実際のGameAppの「Startup error」に未登録シーン名と理由が表示されることをWindowsのUI操作で確認。ダイアログを閉じた後の終了コードは1。ログは`x64/r1-after-fault-run.log`。
- 検証用ソースの変更は元のバイト列へ戻し、通常のGameAppを再ビルド。タイトル画面を目視確認し、Escapeで正常終了（終了コード0）。ログは`x64/r1-normal-run.log`。
- 最終状態: `tools/validate_core.ps1`でDebug／Releaseとも成功。GameApp・AnimationEventEditor・依存ライブラリ、CLI全10種類、WARP、ネイティブ各68件、HLSL4ファイルのVS／PSを検証。ログは`x64/student-template-r1-validation.log`。
- 初回の未登録・factory例外・null・Prepare例外・Activate例外について、原因／段階、診断1件、失敗候補の破棄、更新対象に残らないこと、成功する再試行を確認。通常遷移の旧シーン保持・非同期失敗・再試行は既存テストで維持を確認。
- 未確認: Releaseの実画面操作、ゲーム全体の通し操作、実GPU障害・途中例外時の終了順。R2〜R5とC1〜C7は未実装のまま。次の1コミット分の候補はR2の終了契約の具体化・修正。

`x64/`の検証ログ・TRXはGit管理対象外であり、別PCに同梱されない。実装・テストはCodexによる変更で、学生自身が制作した部分を示すときは提供ひな型の変更として区別する。
