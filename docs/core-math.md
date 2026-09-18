# 数学処理とTransformの所有関係

`src/Framework/Core/Math`には、CPUだけで扱う値型と関数を置いています。標準ライブラリとDirectXMathに依存し、シーン、Win32、GPU資源には依存しません。`tools/check_architecture.ps1`でこの境界を検査します。

## 座標の規約

- 左手座標系です。+Xが右、+Yが上、+Zが前です。
- 角度の単位はラジアンです。`rotationRadians`はXにピッチ、Yにヨー、Zにロールを保持し、DirectXMathの`XMMatrixRotationRollPitchYaw`の規約に従います。
- 行ベクトルの右側に行列を掛けます。変換の順序は、ローカル座標 × 拡大縮小 × 回転 × 平行移動です。ビュー・射影行列は、ビュー × 射影の順です。
- モデルの高さの正規化はモデル読み込み時に行います。アクターのワールド変換では繰り返しません。
- CPUの行列をシェーダーへ渡す際は、HLSLの列優先の格納形式に合わせて転置します。

```cpp
Transform transform;
transform.position = { 2.0f, 0.0f, 3.0f };
transform.rotationRadians.y = DirectX::XM_PIDIV2;
transform.scale = { 2.0f, 1.0f, 1.0f };

auto worldPoint = transform.TransformPoint({ 1.0f, 0.0f, 0.0f });
auto worldDisplacement = transform.TransformDirection({ 1.0f, 0.0f, 0.0f });
DirectX::XMFLOAT3 worldNormal;
bool hasNormal = transform.TryTransformNormal({ 0.0f, 1.0f, 0.0f }, worldNormal);
```

`TransformPoint`は平行移動を含めて変換します。`TransformDirection`は回転と拡大縮小だけを適用し、平行移動や正規化を行わないため、変位の変換にも使えます。`TryTransformNormal`は線形変換部分の逆転置行列を適用して正規化します。法線が不正な場合や、変換が特異または非有限の場合は、出力をゼロにして`false`を返します。

`MathUtils::TryCreateNormalMatrix`は、有限値で表せる逆行列を作れない場合、出力を単位行列にして`false`を返します。テクスチャ描画のパイプラインは、ワールド変換が退化している場合にこの単位行列を代わりに使います。シェーダーでは、法線に逆転置行列、接線にワールド行列の線形変換部分を適用し、ライティングをワールド空間に統一して計算します。

## Transformの所有関係

`StaticMeshActor`と`SkinnedMeshActor`は、それぞれ1つの`Transform`を所有します。位置とヨー角のAPIはこの値を操作し、`GetTransform()`で読み取り専用の参照を取得できます。`StaticModel`と`SkinnedModel`はワールド座標の位置やヨー角を保持しません。`Draw(renderer, world)`でワールド行列を受け取り、エディターも同じ方法で配置を指定します。

直立した形状の衝突判定を持つアクターは、位置とヨー角だけを操作できます。衝突形状が対応していないため、任意のピッチ・ロール・拡大縮小を指定するAPIは公開していません。スキニングを使うアクターの衝突基準点は、アクターの底面からの高さと水平方向のオフセットで扱います。

ルートモーションの移動にも共通の方向変換を使います。`Ignore` / `Blend` / `Apply`の設定、垂直移動の有効化、プログラムによるジャンプの保護は[ルートモーションの設定](root-motion.md)を参照してください。

## 数値処理の仕様

- `TryNormalize`：失敗時は出力をゼロにします。既定の最小長はワールド単位で1e-6です。非有限値や極端な有限値を扱い、入力と出力が同じ変数でも使えます。厳密にゼロかどうかだけで判定する場合は、epsilonに0を渡します。
- `TryNormalizeXZ`：Yを無視し、既定では厳密にゼロかどうかで判定します。
- `NormalizeAngle`：角度を[-pi, pi)の範囲にそろえます。非有限値はゼロになります。
- `LerpAngle`：同じ角度の折り返し規約を使い、最短の回転方向で補間します。
- `SmoothAmount`：小さな時間刻みでも安定する計算で指数平滑化を行います。引数が正でない場合や非有限の場合はゼロを返します。

カメラとCPUスキニングも、個別の実装を持たずにこれらの共通関数を呼び出します。

## テストの実行

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/test_math_core.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/test_root_motion.ps1
```

数学処理のテストは数学処理のコアだけをコンパイルし、拡大縮小・回転・平行移動（SRT）の順序、点と方向の変換の違い、非一様スケールでの法線、不正な入力、角度と平滑化の境界条件を確認します。ルートモーションのテストでは、ジャンプの軌道と着地の動作を確認します。

エディターはモデルを開く要求を次の`BeginFrame`の直前まで保留します。置換後の状態を準備し、送信済みのGPU処理の完了を待ってから旧モデルを解放します。新しいモデルの読み込みに失敗した場合は、現在のモデルとイベント文書を保持します。
