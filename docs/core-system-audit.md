# コアシステム強化の追加調査

2026-09-07追記: この監査に基づく実装内容・検証結果・残る検討は [core-system-improvements.md](core-system-improvements.md) に記録しています。以下は実装前の調査記録です。

調査日: 2026-09-06。対象: `7993f15`（数学コアシステムの強化）時点の自作ソース、エディター、テスト、ビルド設定。

次の大きな強化対象は時間管理と衝突・接地の共通基盤です。その前に、ルートモーションの周回境界と描画バッファの再利用条件を整える価値があります。既存の数学コア、CameraBase、AssetPathResolver、IRenderResourceLifetimeを活用できます。

これは静的なコード調査です。以下の境界条件は実装から判断したもので、ゲーム画面で再現したという報告ではありません。実行時の停止時間・メモリ量は測定していません。プロダクションコードは変更していません。

| 優先 | 対象 | 共通化する場所 | 主な効果 |
| --- | --- | --- | --- |
| 高 | 時間と入力の更新契約 | Core/Time、Frameworkの入力・更新処理 | 停止後や低FPSでも移動・ジャンプを安定させる |
| 高 | ルート変位の周回処理 | Framework/Animation | 一周ちょうど・複数周の移動取りこぼしを防ぐ |
| 高 | GPU使用中のバッファ管理 | Framework/Rendering/Core | 描画数増加時のデータ上書きを防ぐ |
| 中 | 衝突登録・床問い合わせ | Framework/Physics相当 | シーンとキャラクターごとの配線を減らす |
| 中 | 非同期ロード・アセット共有 | Framework/Assets、Models、Rendering | ロード中の停止と同一資源の再生成を減らす |
| 中 | AABB・モデルの原点と高さ調整 | Core/Math、Framework/Models | 描画・ボーン位置・衝突の座標規則を揃える |
| 中 | アニメーションイベント | Framework/Animation | エディターで作ったデータをゲームで利用する |
| 小さく着手可能 | カメラ・パス・数学ヘルパーの置換 | 既存の共通実装 | 重複と規則の不一致を減らす |
| 横断 | 診断とビルド検証 | Core/Diagnostics相当、tools | 原因特定と登録漏れ・回帰の検出を容易にする |

## 1. 時間管理と入力をセットで整備する

[Win32Application.cpp:119](../src/Launcher/Win32/Win32Application.cpp#L119) は実時間差を上限なしでゲームとエフェクトへ渡しています。一方、[SpringFollowCamera.cpp:18](../src/Framework/Scene/Cameras/SpringFollowCamera.cpp#L18) は更新幅を1/30秒に制限しています。ゲームとカメラで、長いフレームの扱いが異なります。

[CharacterGrounding.cpp:154](../src/Game/Gameplay/CharacterGrounding.cpp#L154) の重力積分は更新幅に依存します。既定値のジャンプ速度7、重力-18で、接地中のジャンプ開始更新が0.5秒なら、更新後の速度は-2、変位は-1になります。同じ床が検出されればその更新で着地します。ルートモーションの上下制御とは別に、ジャンプが見えなくなる条件が残っています。

導入候補は、経過時間の蓄積、固定更新幅、最大追いつき回数、中断からの復帰方針を扱うCore/Timeです。OSの時計取得はLauncherに残し、UI・描画とシミュレーションの更新を分けます。

[Input.cpp:9](../src/Framework/Scene/Input/Input.cpp#L9) の押下判定は前後のフレーム状態だけです。固定更新を単純にループ化すると、更新0回のフレームで押下を失い、更新が複数回のフレームでは同じ押下を再使用します。押下・解放を次のシミュレーション更新まで保留し、一度だけ渡す仕組みが必要です。同じ更新内の複数の読み手には同じ入力を渡します。フォーカスを確認せずキー状態を取得する [Win32InputBackend.cpp:39](../src/Framework/Platform/Win32/Win32InputBackend.cpp#L39) も見直し対象です。

移行時には、アニメーション時間・ルート変位抽出・移動・接地を同じ更新刻みで進めます。描画フレームで得たルート変位を複数の更新で再使用してはいけません。水平のみ合成する既定設定と、ジャンプ開始から着地までのプログラムY優先は維持します。

検証候補: 30/60/144FPS相当の入力、長い停止、更新0回・複数回を含む条件で、移動距離、ジャンプ最高点、着地、押下の処理回数を比較する。

## 2. ルートモーション抽出を描画モデルから分離する

[SkinnedModel.cpp:348](../src/Framework/Models/SkinnedModel.cpp#L348) は、次の剰余時刻が前の剰余時刻より小さい場合だけ一周分を補正します。[AnimationSampler.cpp:71](../src/Framework/Animation/AnimationSampler.cpp#L71) も別に剰余時刻を計算しています。

例えば長さ1秒で0から1mへ移動するクリップを時刻0から1秒進めると、両端のサンプル位置が0になり、周回補正も走らず移動量が0になります。2.25秒進めた場合も、完了した2周分を取りこぼします。

Framework/Animationに局所時刻・通過した周回数・区間を扱う再生状態と、描画資源に依存しないルート変位抽出を置きます。ポーズ、ルート変位、イベントが同じ再生区間を参照する構造が適切です。アニメーション固有の意味をCore/Mathへ入れる必要はありません。

現在の [RootMotionTests.vcxproj:41](../tests/RootMotionTests.vcxproj#L41) は合成と接地を対象とし、SkinnedModelの抽出処理はコンパイル対象にしていません。既存テストの成功は、この周回問題を検証したことにはなりません。切り出した処理へ、一周ちょうど・複数周・境界直前直後のテストを追加するのが有効です。

## 3. GPUバッファの再利用をフレームとフェンスに連動させる

[ConstantBufferRing.h:27](../src/Framework/Rendering/Core/ConstantBufferRing.h#L27) は容量に達すると無条件で先頭へ戻り、定数を書き込みます。[BasicColorPipeline.h:25](../src/Framework/Rendering/Pipelines/BasicColorPipeline.h#L25) の容量は1024なので、同じフレームでこのパイプラインを1025回描画すると、最初の描画命令が参照する行列を上書きします。

[Dx12Renderer.cpp:503](../src/Framework/Rendering/Core/Dx12Renderer.cpp#L503) にはフレームのフェンス管理がありますが、リングの領域再利用とは連動していません。現在の少数オブジェクトで破損を観測したという意味ではなく、描画数増加時の明確な制約です。

Rendering/Coreのフレーム資源管理・アップロード領域割り当てへ集約し、GPU完了後だけ再利用します。容量超過はページ追加などで処理し、GPUが使用し終えていない領域を上書きしない契約にします。既存のIRenderResourceLifetimeは廃棄を扱うため、その役割を維持しながら連携させます。

検証候補: 容量ちょうど・容量超過・複数フレーム未完了時の割り当てで、使用中の領域が重ならないことを確認する。

## 4. 衝突登録と床問い合わせをFrameworkへ移す

[GameScene.cpp:98](../src/Game/Scenes/GameScene.cpp#L98) は地面・着地面をプレイヤーへ個別登録し、[GameScene.cpp:238](../src/Game/Scenes/GameScene.cpp#L238) は側面衝突の反復解決を担当しています。側面衝突への登録と、上に乗れる面への登録が別です。

[CharacterGrounding.h:35](../src/Game/Gameplay/CharacterGrounding.h#L35) はGroundとPrimitiveObjectの具象型を保持しています。[PrimitiveObject.cpp:126](../src/Game/Gameplay/PrimitiveObject.cpp#L126) にも別の重力処理があります。このままでは新しいNPC・地形・シーンを作るたびに、同様の接続と更新順を再実装します。

最初は床ヒット情報とICollisionQuery相当の問い合わせAPIを作り、既存GroundとPrimitiveObjectを接続します。その後、登録・除去・押し戻しをCollisionWorldへ移します。純粋な形状計算はCore/Math、ワールドの登録と問い合わせはFramework/Physics、操作感やジャンプ・ルートモーションの選択はキャラクター制御へ分けます。

大規模な物理エンジンやECSの導入は、この改善の前提ではありません。

## 5. 非同期ロードの契約を実際の読込へ適用する

[IScene.h:29](../src/Framework/Scene/Core/IScene.h#L29) のPrepareは空で、現在のGameSceneは上書きしていません。[GameScene.cpp:50](../src/Game/Scenes/GameScene.cpp#L50) のActivateからモデル初期化へ進み、メインスレッドでFBX・画像の読み込みを行います。非同期ロードの枠組みは存在しますが、重い処理の分離が未完了です。

[SceneManager.cpp:219](../src/Framework/Scene/Core/SceneManager.cpp#L219) は旧シーンを先に退役させ、249行のfuture.getと250行のActivateの例外に対する復帰経路もありません。

CPU側のモデル・画像データをPrepareで準備し、ActivateではGPU資源化を行う形へ移します。大きな転送は分割・一括転送を検討します。旧シーンを保持する方式では新旧両方のメモリが必要になるため、その予算と失敗時の復帰画面を決めた上で、成功後に切り替えます。

また、[SkinnedModel.cpp:52](../src/Framework/Models/SkinnedModel.cpp#L52) のmaterialCacheはInitialize内だけの共有です。[Texture2D.cpp:205](../src/Framework/Rendering/Materials/Texture2D.cpp#L205) 以降ではテクスチャごとに転送用のキューなどを作って同期完了を待ちます。

不変なモデルデータ・テクスチャ・マテリアルを共有し、アニメーション時刻・骨行列などの個体状態を分離すると、同じキャラクターを増やした際に効果があります。テクスチャのキャッシュキーはパスだけでなくsRGB等の読込条件も含めます。実際の性能改善量は、読込回数と転送待機時間を測って判断します。

## 6. 数学コアを境界ボックスと座標調整へ広げる

AABBの集計が [StaticModel.cpp:64](../src/Framework/Models/StaticModel.cpp#L64)、[SkinnedModel.cpp:241](../src/Framework/Models/SkinnedModel.cpp#L241)、[SkinnedModel.cpp:412](../src/Framework/Models/SkinnedModel.cpp#L412)、[CollisionComponents.cpp:108](../src/Game/Gameplay/CollisionComponents.cpp#L108) に重複しています。

Core/Mathへ空状態・有限値の扱いを定義したAabbを追加し、点の追加、中心、サイズを共有できます。モデルを足元原点へ揃え、高さを調整する規則はModelsのModelFit相当へまとめます。ActorのワールドTransformと、素材の原点・高さ調整を分ける現在の方針は維持します。

既存コアの適用漏れもあります。[CollisionComponents.cpp:12](../src/Game/Gameplay/CollisionComponents.cpp#L12) のXZ距離計算はMathUtils::LengthXZへ置換できます。

さらに、[CpuSkinnedMeshProcessor.cpp:58](../src/Framework/Animation/CpuSkinnedMeshProcessor.cpp#L58) と [SkinnedTextured.hlsl:61](../src/Framework/Rendering/Shaders/SkinnedTextured.hlsl#L61) のボーン法線変換は通常のボーン行列を使っています。前回直したワールド法線変換とは別で、非一様ボーンスケールを含む素材で照明が不正になる条件があります。スケール対応の方針を決め、既存TryCreateNormalMatrixを使った法線変換とブレンドの規則をCPU/GPUで揃える候補です。単一ボーンの非一様スケールから検証を始められます。

## 7. エディターのアニメーションイベントを実行系へつなぐ

[AnimationEventData.h:20](../tools/AnimationEventEditor/AnimationEventData.h#L20) にイベント型、[AnimationEventJson.cpp:496](../tools/AnimationEventEditor/AnimationEventJson.cpp#L496) に読込処理がありますが、src側にはこのイベント形式の読込・発火処理がありません。

イベントのデータ型・ファイル形式・再生区間に入ったイベントを列挙する処理をFramework/Animationで共有すると、足音・攻撃判定・エフェクトのタイミングをエディターで設定できます。固定長の編集用文字バッファ、選択状態、Undo履歴はツールに残します。

イベントから直接バックエンドを呼ぶ構造にせず、意味を持つイベントをGameへ渡し、既存の音・エフェクトサービスへ接続します。周回、シーク、クリップ切替、境界時刻での重複発火の契約を先に定義し、2項の再生区間を共用します。

保存側はschemaを書きますが、読込側はsourceFbxとevents以外を読み飛ばします。共通化時には形式のバージョン確認と入力検証も整える余地があります。

## 8. 新しい仕組みを作らず置き換えられる場所

- カメラ: [CameraBase.h:9](../src/Framework/Scene/Cameras/CameraBase.h#L9) がある一方、[Camera.h:8](../src/Framework/Scene/Cameras/Camera.h#L8) と [FollowCamera.h:7](../src/Framework/Scene/Cameras/FollowCamera.h#L7) はレンズ・ビュー・前方方向を別に実装しています。実際に使われるFollowCameraを先にCameraBaseへ移し、追従計算と初期姿勢を維持できます。
- パス: [ModelTextureResolver.cpp:58](../src/Framework/Models/ModelTextureResolver.cpp#L58) は文字列からfilesystem::pathを直接作り、string()で返します。一方、[AssetPathResolver.cpp:17](../src/Framework/Assets/AssetPathResolver.cpp#L17) は入力をUTF-8として変換します。非UTF-8のWindows環境では日本語パスが不一致になる条件があります。既存Assetsの変換規約を公開・共有し、内部はfilesystem::path、ライブラリとの境界で明示変換する構造に揃えます。現在の素材での発症確認はしていません。
- 診断: [Common.h:8](../src/Framework/Common/Common.h#L8) のThrowIfFailedはHRESULTの値や操作名を失います。[Texture2D.cpp:109](../src/Framework/Rendering/Materials/Texture2D.cpp#L109) は例外をまとめて代替画像に変え、原因を記録しません。共通の診断情報に操作名・資源パス・エラー値を含め、出力先を交換できるようにすると調査が容易になります。Windows文字列変換やHRESULT処理自体はプラットフォーム側に残します。

ボーンのクォータニオンをEuler角版Transformへ変換したり、単純なDirectXMath呼び出しを一律にラップしたりする必要はありません。責務と数値規則の共有に効果がある箇所を優先します。

## 9. コア拡張を支える検証をまとめる

[EngineFramework.vcxproj:37](../EngineFramework.vcxproj#L37) のビルド前チェックは依存関係を対象とし、ソースの登録漏れは検査しません。[sync_vs_filters.ps1](../tools/sync_vs_filters.ps1) に再生成機能はありますが、実行し忘れを検出する仕組みが必要です。

[EduGame3D.sln:5](../EduGame3D.sln#L5) は4つの本体プロジェクトだけを含み、数学・ルートモーションのテストは独立スクリプトです。シェーダーもNone項目で、C++のビルド成功だけではコンパイルされません。

再生成スクリプトへ読取専用の整合性チェックを追加し、依存関係・プロジェクト登録・Debug/Releaseビルド・回帰テスト・シェーダー検証を一つの検証コマンドから実行できるようにすると、前回のリンクエラーの再発を検出しやすくなります。ビルド中にプロジェクトを書き換える方式より、事前に不一致を報告する方式が扱いやすいです。

今回実施した確認:

- tools/check_architecture.ps1: 成功。
- 自作cppのプロジェクト登録: Framework 55、Game 17、Launcher 2、Editor 5、計79ファイルに登録漏れなし。
- ソース調査のみのため、今回の全体再ビルド、ゲームの手動操作、負荷測定は未実施。

実装順は、まず周回境界とGPU領域再利用の個別修正・回帰検証、次に時間管理と入力、その後に衝突・接地、アセット読込と共有です。AABBとCameraBaseへの置換は、挙動を保ちながら独立して進めやすい作業です。アニメーションイベントは再生時間の契約を整えた後につなぐと、同じ境界問題を二重に実装せずに済みます。
