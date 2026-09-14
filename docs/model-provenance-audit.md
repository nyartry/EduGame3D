# EduGame3D：3Dモデルの取得元・配布条件の調査

確認日: 2026-09-14。対象: **置換前の** `Content/Models`、基準コミット `73cb3279ddfa08f13d96214419cbc46524391a9c`。

**置換後の状態:** 本調査の後、所有者の依頼で外部モデル・付属画像・モーションを現在のソースツリーから除外し、新規制作のEduHumanへ切り替えました。以下の「現在」「現行」は基準コミット時点の記録です。[置換記録](model-replacement.md) と [制作記録](../Content/Models/EduHuman/PROVENANCE.md) が現在の構成を説明しています。照合用ハッシュは旧素材の混入検出に使うため保持します。

**名称変更は完了していますが、現在のモデルをそのまま含めたPublic化・教材配布はまだ準備完了と判断できません。** 無料取得、完成作品への利用、編集可能なFBX・画像の再配布は別の許諾です。以下は取得元の証拠と公開準備上の判断であり、モデルに新しいライセンスを付与するものではありません。

## 1. 置換前の調査結論

| 対象 | 取得元・制作者の判断 | 公開用プロジェクトでの扱い |
|---|---|---|
| `Player/Jogging`、`Player/Orc Idle` | Mixamo。FBX内部情報に加え、Windowsの取得元記録も確認。 | 原FBXの公開同梱は保留。利用者自身による公式取得、再配布可能な別素材、または明示的な許可を検討。 |
| `Player/Mma Kick`、`Y_Bot` | Mixamo経由のデータである確度が高い。FBX内部に制作サービスの情報。取得URLの記録はなし。 | 上記と同じ。キャラクターの元の権利までサービス名だけで確定しない。 |
| `Daven` | CGTraderの `Daven Male Rigged` が有力。TurboSquidにも同じ説明・画像の配布ページ。実際の取得サイトは未確定。 | 現行の通常ライセンスを、生FBX・画像の公開配布許可として扱わない。公式取得方式・別素材・個別許可を検討。 |
| `55-rp_nathan_animated_003_walking_fbx` | Renderpeopleの `Nathan Animated 003 - Walking 3D Man` と高い確度で対応。複数の配布経路がある。 | **公式SketchfabにCC BY 4.0版あり。** その配布物とファイルを対応づけ、表示条件を整えれば同梱候補にできる。手元の8ファイルへの適用は未確定。 |
| `Untitled/Untitled.fbx`、`old_Untitled.fbx` | **ユーザーが両方とも自分で作ったと確認済み。** Git履歴・Blenderの内部情報とも整合。 | 自作のサンプルとして保持する候補。制作元データと作者の記録を残す。 |

モデルフォルダーにはFBX 10点、外部画像163点、イベントJSON 2点、README 1点があります。FBXのSHA-256・サイズは [照合用記録](model-provenance-fingerprints.json) に保存しました。ハッシュはファイル識別用であり、許諾の証拠ではありません。

## 2. Mixamoの4点

4FBXに `Mixamo, Inc.`、`mixamo.com` とサービス側のエクスポート情報が残っています。さらに `Jogging.fbx` と `Orc Idle.fbx` のNTFS付加情報 `Zone.Identifier` に、参照元 `https://www.mixamo.com/` とMixamoのエクスポート先ホストがありました。署名付きURLのクエリは記録・転載していません。これらの付加情報は通常Gitには保存されません。

Adobeの [公式Mixamo FAQ](https://helpx.adobe.com/creative-cloud/faq/mixamo-faq.html) は、モデル・アニメーションの商用／非商用のゲーム等への利用を案内しています。一方、[Adobe一般利用条件3.6節](https://www.adobe.com/legal/terms.html) は、コンテンツを最終作品に組み込んで利用する許諾と、単体配布の制限を定めています。FAQの「商用利用可」だけでは、学生が再利用するための原FBXを配る根拠になりません。

[Adobe Communityの配布に関するFAQ投稿](https://community.adobe.com/questions-696/mixamo-faq-licensing-royalties-ownership-eula-and-tos-589400) にも原データを配るテンプレート等への制限の説明がありますが、これはCommunityの投稿であり、契約本文と区別します。今回の公開判断は一般利用条件と実際の配布形態を基にし、必要なら原データを含む教育用フレームワークについてAdobeへ確認します。

## 3. Davenの取得元候補

ローカルの `Daven/t-pose.fbx` はBlender 3.6.1の書き出し情報を持ち、元ファイル名 `Jack.blend`、制作元パス中の名前 `d_e_c`、`t-pose.fbm` のテクスチャ参照を含みます。公開資料には個人の絶対パス全体を転載しません。

| 候補 | 照合できた情報 |
|---|---|
| [CGTrader：Daven Male Rigged](https://www.cgtrader.com/free-3d-models/character/man/daven-3d-model) | 作者 `d-e-c`、ID `4943188`、2023-12-03公開、無料、Royalty Free License (no AI)。Blender 3.6・FBX・T-poseの説明が内部情報と整合。 |
| [TurboSquid：Daven Blender](https://www.turbosquid.com/3d-models/3d-daven-blender-model-2160053) | 作者表示 `OM 3D`、ID `2160053`、同日公開、無料、Standard License。同じ説明・画像の掲載。 |

特に公開作者名とFBX内部の名前の一致は強い手掛かりです。ただし、当時のZIP・取得履歴と照合していないため、取得サイトや契約は確定していません。公開ページからZIP内の全ファイル一覧やハッシュも確認できませんでした。

[CGTrader公式説明](https://help.cgtrader.com/hc/en-us/articles/360015124437-Royalty-Free-License) と [規約21A節・24節](https://www.cgtrader.com/pages/terms-and-conditions) は、無料素材にも選択されたライセンスが適用されること、組込み作品の条件、素材へのアクセスを防ぐ措置を定めています。生FBX・画像が取得できる現在のソース配布方式は、その通常許諾で公開できるとは判断しません。

[TurboSquidのライセンス](https://www.turbosquid.com/licensing) も、II.7(b)・(c)で素材へのアクセスと汎用ツールの素材集への同梱を制限しています。同ページの教育利用説明では、学生の個人PCへの原モデルの配布を認めず、無料モデルを学生自身が取得する方法を案内しています。

`CC_Base_Body` や髪・服の名前にはCharacter Creator系の制作素材の手掛かりもありますが、これだけで権利者や無断配布を断定しません。Davenについて個別許可を求める場合は、ベースモデル・髪・服・テクスチャを含む配布権限も確認対象です。モデルをBlenderで開いて保存し直すだけでは、第三者の権利が消えるわけではありません。

## 4. Nathanは配布経路によって条件が異なる

通常版・`_u3d`・`_ue4` の3FBXにRenderpeopleの制作パスと同じモデル識別名が残っています。3FBXとJPEG 5点は2026-05-15にまとめて導入されています。取得サイトの記録はありません。

| 確認した配布経路 | 現在の表示・根拠 |
|---|---|
| [Renderpeople公式Sketchfab](https://sketchfab.com/3d-models/nathan-animated-003-walking-3d-man-143a2b1ea5eb4385ae90a73657aca3bc) | 投稿者 `renderpeople`、CC Attribution。[公開API](https://api.sketchfab.com/v3/models/143a2b1ea5eb4385ae90a73657aca3bc) でも `isDownloadable: true`、ライセンスURL `http://creativecommons.org/licenses/by/4.0/` を確認。 |
| [Renderpeople公式ArtStation店](https://renderpeople.artstation.com/store/pqGL/nathan-animated-003-walking-3d-man) | 無料、Extended Commercial License。配布ZIP名は `rp_nathan_animated_003_walking_FBX`、48 MB。 |
| [Free3DのRenderpeople投稿](https://free3d.com/3d-model/nathan-animated-003-walking-644277.html) | 同名モデル・ZIP。現在の表示はPersonal Use License。過去の取得条件までは確定しない。 |
| [Renderpeople本家](https://renderpeople.com/free-3d-people/) | 無料モデルにも本家の利用条件が適用されると説明。 |

本家の通常条件は、ゲーム等への利用と、個別ファイルの再配布・学習環境への組込を区別しています。無料だから原FBXをそのまま配れるという扱いにはできません。[本家規約2.5・4.1～4.3節](https://renderpeople.com/general-terms-and-conditions/)

一方、**CC BY 4.0版は公開教材に適した選択肢です。** [CCの日本語説明](https://creativecommons.org/licenses/by/4.0/deed.ja) と [ライセンス本文](https://creativecommons.org/licenses/by/4.0/legalcode.en) は商用を含む複製・再配布・改変を認め、作者・出典・ライセンス・変更内容等の表示、提供された通知の保持、許諾を妨げる追加制限を課さないことを求めています。

次は公式CC BY配布物にファイルを揃えるか、手元の各FBX・テクスチャがその許諾範囲に入ることを照合します。配布物を確認していない現段階で、手元の8ファイルへ一括してCC BY表示を付けることはしません。

## 5. Untitledの本人確認と導入履歴

ユーザーは2026-09-14、この調査中に `Untitled.fbx` と `old_Untitled.fbx` を「自分で作った」と回答しました。2026-05-30の導入コミットも自作スキンメッシュと説明しています。旧版のFBXには制作元 `Untitled.blend` の情報があり、旧版のGit blobは初版と一致します。制作元 `.blend` はリポジトリでは確認できませんでした。

| ファイル群 | 最初の導入・変更 |
|---|---|
| Daven | `8c90e61`、2026-05-09。以後FBXの変更なし。 |
| Jogging | `0199fcb`、2026-05-10。同日 `d093c15` で現在の場所へ内容を変えず移動。 |
| Orc Idle | `d093c15`、2026-05-10。 |
| Nathan 3点 | `9457b10`、2026-05-15。 |
| Mma Kick | `1c30428`、2026-05-15。 |
| Untitled | `067fcd2`、2026-05-30。`2be4776`、2026-06-03にFBXスケール設定を修正し旧版を保存。 |
| Y_Bot | `db9a391`、2026-06-12。 |

## 6. 公開までの具体的な作業

1. **標準サンプルを、自作・再配布許諾を確認できる素材で動く構成にする。** Untitledは自作の候補として保持する。MixamoとDavenは同梱を前提にせず、利用者が公式サイトから取得する追加サンプル、別素材への置換、個別許可のいずれかにする。
2. **Nathanは公式CC BY版を採用するかを決め、版とファイルを対応づける。** 取得日・出典・ライセンス・対象ファイル・改変内容を記録し、必要な表示を配布物へ同梱する。
3. **モデルを外す場合はコードも対応する。** [GameScene](../src/Game/Scenes/GameScene.cpp#L62) はOrc・AnimatedCube・Daven・Nathanを生成する。Orcの置換は待機・移動・攻撃の3クリップにも影響する。Untitledは [モデル準備テスト](../tests/ModelPreparationTests.cpp#L60) でも使うため、一括削除しない。
4. **公開するGit履歴も確認する。** 削除済み `forest_goddess` のFBX・画像計27点が、`6759670` で追加、`94bcd6a` で削除された履歴に残る。取得元・条件は未確定。現在のフォルダーから削除したり `.gitignore` を追加したりするだけでは履歴から外れない。公開用に新しい履歴を作る方法などを検討する。
5. **許諾済みファイルだけで、新規取得・ビルド・ゲームとエディターの起動を確認する。** プロジェクト全体のLICENSEとは別に、素材の作者・ライセンス・出典を記録する。

今回行ったのはローカル資料・FBX情報・取得元付加情報・Git履歴・公開された配布ページと規約の調査です。モデルの外部アップロード、再取得、削除、差し替え、Git履歴の変更、Public化は実施していません。ダウンロード履歴・購入履歴・当時のZIPは未確認です。取得先の確定に必要なら、まず2026年5月9日～6月12日前後の記録が候補になります。
