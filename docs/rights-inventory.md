# 権利台帳：公開・学生配布に向けた調査初版

調査日: 2026-09-14。対象: `e0714173e9e07d7ba5d625bb5727d54f414cd6af`。
名称更新: プロジェクト名は **EduGame3D**。リンク先とA10のファイル名は調査後の名称変更に合わせています。A10の画像内容は変更していません。
作業の優先順位とSARTRAS制度上の整理は [調査報告](../docs/sartras-readiness-audit.md) を参照。

この台帳は確認済みの事実と不足資料を記録するもので、利用許諾を新たに与える文書ではありません。「未確認」は違反の認定を意味しません。バイナリの制作者文字列やファイル名は出所の手掛かりであり、本人が取得したライセンスの証拠とは区別します。

## 1. 自作候補のコード・資料

| ID | 対象 | 確認できたこと | 未確認／必要な証拠 | 対応 |
|---|---|---|---|---|
| O01 | `src/Framework/` 157ファイル | 共通API・エンジン実装。区分は [architecture.md 17行](../docs/architecture.md#L17)。 | 実際の著作者、職務著作・共同制作・譲渡、初期コードの参考元・転用、人の創作的寄与。 | R01、R02、R04 |
| O02 | `src/Game/` 35ファイル、`src/Launcher/` 3ファイル | サンプル規則・コンテンツ指定・起動処理。 | 著作者・権利者。参照する素材は別権利として下表に分離。 | R01、R03、R04 |
| O03 | 既存 `docs/` 12ファイル、README | 数学・アニメーション等の解説とコード例。権利者表示・配布版は未整備。 | 解説文・図・コード例それぞれの制作過程と出典、公開名義。 | R01、R02、R07、R08 |
| O04 | `tools/` 24ファイル、`tests/` 30ファイル | エディター、検証スクリプト、回帰テスト。 | 制作者と権利範囲。ImGui等の取り込み部分を本人のコードと混同しない。 | R01、R02、R05 |
| O05 | [SpriteBatch.cpp 17行](../src/Framework/Rendering/Sprites/SpriteBatch.cpp#L17) の5×7文字表 | コード内に文字パターンを保持。 | パターンの制作者・参考元。この存在だけで独創性や権利の帰属を判断しない。 | R01 |

作者ラベルはGit履歴に4種類ありますが、同じ人の別名義の可能性があります。全130コミットのラベルを実際の制作者と対応づけます。Gitの名義だけで権利者を決めません。

[引き継ぎ資料337行](../docs/student-template-core-handoff.md#L337) は、その節の実装・テストへのCodexの関与を記載しています。AIを使ったことだけで全体の権利の有無は決まりません。創作的な表現に関する指示・人の修正等を実際の記録から確認します。[文化庁の説明](https://www.bunka.go.jp/seisaku/bunkashingikai/chosakuken/pdf/94057901_01.pdf)

## 2. 外部コード・依存バイナリ

`third_party` は5ディレクトリ、418追跡ファイル。うち20個の `.lib` / `.dll` は [.gitattributes 8行](../.gitattributes#L8) 以降でGit LFS管理。以下のライセンス名はローカル本文を根拠にしています。

| ID | 同梱物 | 保存されている根拠 | 不足している作業 |
|---|---|---|---|
| D01 | Assimp、Debug/Release DLL・LIB | [version.h 6行](../third_party/assimp/include/assimp/version.h#L6) に著作権・BSD 3-Clause相当の本文。[revision.h 4行](../third_party/assimp/build_include/assimp/revision.h#L4) に `116c96c4`、同7行以降に6.0.5。 | 独立したLICENSEの同梱、採用版とバイナリの対応、組み込まれた二次依存・改変の有無、実行物用の通知を確認。ヘッダー中には許諾本文があり「ライセンス全部欠落」ではない。 |
| D02 | DirectXTK12 | [LICENSE](../third_party/directxtk12/LICENSE#L1) はMicrosoftのMIT。[README 3行](../third_party/directxtk12/README.md#L3) 以降に上流、`e656d54`、採用ビルドと構成。 | 既存の出所記録を維持し、実行物への通知同梱に接続。この記録方法を他の依存にも展開。 |
| D03 | Effekseerと各Renderer | [LICENSE.txt](../third_party/effekseer/LICENSE.txt#L1) はMIT。 | 採用revision、ビルド手順、組み込まれた依存と告知範囲を確定。素材のライセンスは別表。 |
| D04 | LLGI（Effekseer側に同梱） | [GameApp.vcxproj 19行](../GameApp.vcxproj#L19)・24行でリンク。ヘッダー・LIBが存在。 | LLGI自体の採用版とライセンスの対応を確認。[LICENSE_RUNTIME_DIRECTX.txt 22行](../third_party/effekseer/LICENSE_RUNTIME_DIRECTX.txt#L22) 付近には旧DirectX Tool KitとMs-PLの記述がある。別同梱のDirectXTK12はMITであり、対象コードを照合せず表記を削除しない。 |
| D05 | RmlUi | [LICENSE.txt](../third_party/rmlui/LICENSE.txt#L1) はMIT。コンテナ用の [LICENSE.txt](../third_party/rmlui/include/RmlUi/Core/Containers/LICENSE.txt#L1) にrobin_hood／itlibのMIT本文。 | 両方の告知の同梱、revision・ビルド条件・オプションを記録。 |
| D06 | Dear ImGui | [LICENSE.txt](../third_party/imgui/LICENSE.txt#L1) はMIT、[imgui.h 32行](../third_party/imgui/imgui.h#L32) の版は1.92.9 WIP。 | 本体に加え以下のフォント・stbの通知を整理。エディター配布にも含める。 |
| D07 | ImGui内のフォント・stb | [imgui_draw.cpp 6373行](../third_party/imgui/imgui_draw.cpp#L6373) にProggyClean、[同6563行](../third_party/imgui/imgui_draw.cpp#L6563) にProggyForeverのMIT表示。stb本文は [imstb_rectpack.h 591行](../third_party/imgui/imstb_rectpack.h#L591)、[imstb_textedit.h 1491行](../third_party/imgui/imstb_textedit.h#L1491)、[imstb_truetype.h 5049行](../third_party/imgui/imstb_truetype.h#L5049)。 | 本体LICENSEだけにまとめず、採用部分の著作権・許諾を保持。stbは記載された選択許諾に従って扱う。 |

RmlUi上流READMEにFreeTypeやDebuggerフォントの説明はありますが、実装は [独自フォントエンジン](../src/Framework/UI/RmlUi/RmlUiService.cpp#L32) を使用しています。Release LIBの代表シンボル調査でもそれらの混入は確認できず、ライセンス欠落を確定問題として挙げません。バイナリのビルド構成確認に含めます。

GameAppとエディターの現在のPostBuildには、通知文書のコピーがありません。外部コードの全文がソースツリーにあることと、EXEフォルダだけで必要表示が揃うことは別に確認します。[GameApp.vcxproj 20行](../GameApp.vcxproj#L20)、[AnimationEventEditor.vcxproj 20行](../AnimationEventEditor.vcxproj#L20)

Assimp DLLはVCランタイムにも依存します。EXEを配布する場合はReleaseの動作環境・正規のランタイム入手手順を整え、再頒布する場合のみその条件を確認します。MicrosoftのランタイムDLL自体を現在同梱しているとの証拠はありません。

## 3. 素材と生成物

現在の `Content` は186ファイル。FBX10、画像167（JPG93・PNG66・TGA8）、WAV3、エフェクト1、イベントJSON2、README2、`.gitkeep`1。独立したフォントファイルは確認できませんでした。

| ID | パス／まとまり | 出所について確認できたこと | 公開・学生配布までに必要なこと |
|---|---|---|---|
| A01 | `Content/Models/Player/Jogging/Jogging.fbx` | FBX内部にMixamoの表記。 | 取得時条件・元モデルの権利・生FBXの配布可否を確認。 |
| A02 | `Content/Models/Player/Mma Kick/Mma Kick.fbx` | FBX内部にMixamoの表記。[エディターREADME 57行](../tools/AnimationEventEditor/README.md#L57) にもクリップ情報。 | A01と同様。素材変更時はイベント教材も追従させる。 |
| A03 | `Content/Models/Player/Orc Idle/Orc Idle.fbx` | FBX内部にMixamoの表記。 | アニメーション／リグとキャラクター本体の権利を分け、取得・再配布条件を確認。 |
| A04 | `Content/Models/Y_Bot/Y_Bot.fbx` | FBX内部にMixamoの表記。 | テンプレートへの生データ同梱条件を確認。必要なら利用者取得方式・別素材にする。 |
| A05 | `Content/Models/55-rp_nathan_animated_003_walking_fbx/` 8ファイル | FBX3点の内部にRenderpeopleの制作元パス。 | 実際の取得元・契約を確認。個別ファイルを学生やPublic GitHubへ配布できる根拠がなければ、置換／同梱除外等を決める。 |
| A06 | `Content/Models/Daven/` 159ファイル（FBX1、画像158） | FBXにBlender exporter情報。画像名だけでは販売元や許諾を特定できない。 | 制作者、モデル・衣装・テクスチャの取得元と適用規約を確認。Blenderからの出力は権利の証明ではない。 |
| A07 | `Content/Models/Untitled/Untitled.fbx`、`old_Untitled.fbx` | `067fcd2` の導入説明に自作スキンメッシュとある。制作元 `.blend` は追跡されていない。 | 自作の手掛かりを本人に確認し、制作元データ・作者・権利者を記録。旧版を公開対象に含めるか決める。 |
| A08 | `Content/Audio/BGM/title_theme.wav`、`game_theme.wav`、`Content/Audio/SE/button_click.wav` | [GameContent.h 22行](../src/Game/Content/GameContent.h#L22) 以降で使用。導入履歴は音声再生の実装説明のみで、音源の権利資料はない。 | 3音源それぞれの制作者・取得元・再配布条件を記録。AI生成なら制作過程・使用サービスの条件も確認。 |
| A09 | `Content/Effects/Effekseer/Samples/Laser01.efkefc` とテクスチャ3点 | `db9a391` で導入。元パッケージ・版の記録はない。 | 公式サンプルの取得版とファイルを対応づけ、素材のCC0等の適用を確認。ランタイムMITだけを素材の根拠にしない。 |
| A10 | `edugame3d_crest.png` | [GameScene.cpp 31行](../src/Game/Scenes/GameScene.cpp#L31) の変数名は `GeneratedImageTexturePath`。 | 実際の制作者・生成経緯・利用条件を記録。変数名だけでAI生成物や自作と断定しない。 |
| A11 | `Mma Kick.anim_events.json`、`Y_Bot.anim_events.json` | アニメーションに対応するイベントデータ。 | データの制作者を確認し、元アニメーション変更時には内容・教材との整合を確認。 |
| H01 | 過去の `Content/Models/forest_goddess/` | `94bcd6a` で削除されたFBX1点と画像26点、計27点がGit履歴に存在。 | 現在ツリーだけでなく、公開する履歴とLFSの範囲も確認。公開用成果物の構成を決める。今回履歴操作はしていない。 |

Mixamoの表記はサービスの関与を示しますが、アップロード元キャラクターも含めた権利を証明するものではありません。Adobeの [Mixamo公式FAQ](https://helpx.adobe.com/creative-cloud/faq/mixamo-faq.html) はゲーム等への利用を案内し、[一般利用条件3.6節](https://www.adobe.com/legal/terms.html) はContent Filesの単体配布を制限しています。学生向けソースひな型として元ファイルを渡す場合の適用条件を確認する必要があります。

Renderpeopleの現行条件は無料素材にも適用され、第三者への譲渡・再許諾や、個別の素材を容易に取得できる提供を制限しています。A05の取得時条件・個別契約は未確認なので、公開前の優先確認対象とします。[Renderpeople利用条件2.5、4.2、4.3節](https://renderpeople.com/general-terms-and-conditions/)

Effekseer公式概要はランタイムと素材のライセンスを区別しています。A09が当該条件の対象ファイルであることを元パッケージと照合します。[公式概要](https://effekseer.github.io/Help_Tool/ja/overview.html)

現状の [GameScene.cpp 62行](../src/Game/Scenes/GameScene.cpp#L62) 以降はOrc、Daven、Nathan等を生成・準備します。素材を削除するだけではサンプルが動かなくなる可能性があるため、置換時は再配布可能な最小サンプルを用意して起動・教材を検証します。[AssetPathResolver.cpp 53行](../src/Framework/Assets/AssetPathResolver.cpp#L53) は親ディレクトリも探索するため、配布フォルダをソースツリー外へ置いた検証が必要です。

## 4. 台帳を確定するための記録項目

各行は、取得元や条件が異なる場合にはファイル単位に分割します。添付書類・購入記録・個人情報をこの公開予定の文書へ直接埋め込まず、証拠の管理先を参照します。

| 項目 | 初版での扱い |
|---|---|
| ファイル範囲・作品名・版 | 上表の範囲を起点に特定。複数の権利があるモデル等は分ける。 |
| 著作者・権利者 | 未確定。自作／外部の推定だけで個人名を記入しない。 |
| 取得元URL・取得日・原本 | 本人の記録で補完。可能なら原本のハッシュと規約版も保存。 |
| 加工・AI支援・人の創作的寄与 | 実際の制作記録と変更差分を対応づける。 |
| 適用ライセンス・条件 | 採用した原本・版に適用される本文へ参照を付ける。 |
| 生ファイルの再配布 | Public GitHub、学生向け限定配布それぞれについて可否と根拠を記録。 |
| 完成作品の配布 | EXE・動画・就職作品への掲載等について条件を記録。 |
| 必要なクレジット・通知 | 表記内容、本文、同梱先を対応づける。 |
| SARTRASで本人が申し出る範囲 | 本人が権利を持つ部分と第三者部分を区別。対象かどうかは利用実態等で判断。 |
| 採否・完了証拠 | 許可確認済みで採用／置換／除外／追加許諾確認中のいずれかと根拠。 |

ソースやバイナリのライセンス確認、素材の許諾確認、SARTRASの分配判断はそれぞれ別の確認です。全行が整理できたことだけで補償金受給が確定するものではありません。
