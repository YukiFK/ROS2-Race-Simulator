# REVIEW_CONTEXT

## 役割
この文書は、この repo における**不変寄りのレビュー原則の正本**です。  
現在進捗や一時的な作業状況は `docs/status/PROJECT_STATUS.md` に置き、この文書にはレビュー判断に効く恒久ルールだけを残します。

---

## レビューの基本原則

### 1. build 成功だけでは合格にしない
最低限見る順番は次のとおり。

1. 仕様一致
2. 差分境界
3. 責務分離
4. validation 可能性
5. build / test 成否

### 2. issue 単位で小さく進める
- 1 PR 1 論点を基本とする
- 複数論点を混ぜない
- 先回り実装を避ける
- 今回の issue の外にある改善は原則として入れない

### 3. ChatGPT と Codex の役割を混ぜない
- ChatGPT: docs 構成、レビュー観点、進捗整理、PR 差分レビュー
- Codex: repo 内の実作業
- 人間: 最終判断、コミット判断、merge 判断

### 4. repo 内 docs を正本とする
矛盾がある場合は repo docs を優先する。  
チャット内の要約や過去プロンプトは参照用であって正本ではない。

---

## レビューで重視する観点

### 差分境界
- 今回の issue に必要な変更だけか
- 無関係な整形や命名変更を混ぜていないか
- future scope を先取りしていないか

### 責務分離
- race-wide responsibility と vehicle-local responsibility が混ざっていないか
- publisher / coordinator / runtime の責務境界が崩れていないか
- docs / design / status の役割が混ざっていないか

### 仕様一致
- 既存 docs に書かれた current behavior と一致しているか
- 実装以上の仕様を PR 本文や docs に書いていないか
- 未確定を事実のように書いていないか

### validation
- 手元で検証可能な観測点があるか
- `dev_validation.md` か PR 本文で再現手順が追えるか
- monitor / topic / test のどこで確認したかが明確か

---

## 指摘の優先度

レビュー結果は原則として次の 3 区分で扱う。

### 致命
merge 不可。必ず直す。
例:
- 仕様逸脱
- race-wide / vehicle-local の責務混在
- validation 不能
- build / test 破壊
- 無関係な大規模差分混入

### 要修正
この PR の中で直した方がよい。
例:
- issue scope は守っているが API が雑
- docs と実装の不一致
- 不要な CMake 差分
- 不明瞭なログ / 命名 /テスト不足

### 任意改善
今回は見送り可。次 issue 以降でもよい。
例:
- 将来リファクタ候補
- 命名の微改善
- docs の表現調整
- 軽微な読みやすさ改善

---

## CMakeLists.txt の制約

Codex 向けプロンプトでは毎回これを明記する。

- 今回必要な source/test/target の追加以外は変更しない
- 既存の `target_link_libraries(...)` の PUBLIC/PRIVATE を変更しない
- 既存の `ament_target_dependencies(...)` の形を変更しない
- 単なる整形、改行変更、順序変更をしない
- 今回のタスクに無関係な CMake 差分を作らない

レビュー時はまず `git diff <path>/CMakeLists.txt` を確認する。

---

## docs レビューの原則

### specs
- 実装照合用の正本
- current behavior を優先
- 将来仕様を確定しない

### design
- 設計判断と未確定論点の整理
- 未実装を未実装のまま書く

### process
- 再現可能な手順と運用ルール
- 人と AI が同じ手順を踏めることを重視

### status
- 現在進捗と次アクション
- 日報化しない
- 長期仕様を混ぜない

---

## レビュー入力として欲しいもの

差分レビュー時は原則として次を揃える。

- branch 名
- issue の目的
- `git status --short`
- `git diff --name-only`
- `git diff`
- 必要なファイルの `cat`
- 実施した validation コマンド
- Codex の報告要約（ある場合）

---

## 大きい差分の扱い

差分が大きい場合は、次のどちらかで分割する。

- ファイル単位
- 論点単位

分割後も 1 回のレビュー対象で「何を判断する回なのか」を固定する。  
一度に全部見ようとして scope を崩さない。
