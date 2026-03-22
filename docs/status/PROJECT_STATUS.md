# PROJECT_STATUS

## 役割
この文書は、この repo の**現在進捗の正本**です。  
仕様書ではなく、今どこまで実装・整理が完了していて、次に何をやるべきかを把握するために使います。

- 可変情報を置く
- 仕様の正本は `docs/specs/` を優先する
- 設計判断の補助情報は `docs/design/` を優先する
- 運用ルールは `docs/process/` を優先する

矛盾がある場合は:
1. `docs/specs/`
2. `docs/process/`
3. この文書
の順で優先します。

---

## 現在の実装到達点

### 完了済み
current demo は **first multi-vehicle implementation slice** として、次が repo に取り込まれている状態。

- fixed participating vehicles は 2 台
  - `demo_vehicle_1`
  - `demo_vehicle_2`
- `RaceCoordinator` が race-level owner として存在する
- `RaceCoordinator` は 2 台分の `SingleVehicleRuntime` を所有できる
- global `START` / `STOP` / `RESET` は 2 台の runtime に race-wide に適用される
- `VehicleRaceStatus` は 2 台分 publish される
- `LapEvent` は 2 台分 publish される
- race-wide completion は `all participating vehicles finished` で成立する
- docs は first multi-vehicle slice 前提に更新済み
  - validation
  - review checklist
  - milestone1
  - README 導線

### 現在の観測責務
- `RaceState`: race-wide state の観測
- `VehicleRaceStatus`: vehicle-local progress の主観測
- `LapEvent`: vehicle-local lap crossing event の観測

---

## 現在の重要な前提

- issue 単位で小さく実装する
- Codex に実装させる
- ChatGPT はレビューと進捗整理を担当する
- PR は基本 squash merge
- build が通るだけで OK にしない
- 差分境界・責務分離・仕様一致・validation を重視する
- 将来機能の先回り実装は避ける
- repo 内 docs を正本とする
- チャット要約は参照用であり、正本ではない

---

## 現在の非スコープ

まだ current demo / first slice の外にあるもの:

- ranking / winner determination
- leaderboard / result table
- DNF / DNS / disqualification
- dynamic join / leave
- 3 台以上への一般化
- race manager の独立 node 化
- `RaceState` schema redesign
- `RaceState.completed_laps` の最終整理
- GUI / Foxglove 最適化
- planner / control integration
- collision / overtaking

---

## いま注意すべき実装上の暫定点

### `RaceState.completed_laps`
current implementation では、race-wide aggregate ではなく **primary vehicle snapshot 由来**。  
race-wide completion の判断は `race_state status` を主に見て、vehicle-local finish は `VehicleRaceStatus.has_finished` を見る。

これは未整理のまま残っている論点であり、**もっともらしく補完しない**こと。

---

## 次に着手すべき候補

優先順の推奨:

1. `RaceState.completed_laps` の扱い整理（docs or small design issue）
2. first slice の integration test 追加
3. 3 台以上/generalization に進む前の interface 整理
4. race manager / coordinator API の再整理

---

## 現在レビュー中 / 作業中

現時点で未確認の active diff:
- なし

次に着手する issue:
- TBD

---

## 更新トリガー

この文書は次のタイミングで更新する。

- issue 着手時
- 方針変更時
- PR 作成時
- PR merge 後
- 次チャットへ移る前
