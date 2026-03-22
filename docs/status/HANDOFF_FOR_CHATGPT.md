# HANDOFF_FOR_CHATGPT

## 役割
新チャット開始時に、最小限の文脈を短く引き継ぐための文書。  
詳細は repo 内 docs を読ませる前提で、ここでは**冒頭で必要なことだけ**をまとめる。

---

## 現在の状態
- current demo は first multi-vehicle implementation slice
- participating vehicles は固定 2 台
  - `demo_vehicle_1`
  - `demo_vehicle_2`
- `RaceCoordinator` が race-level owner
- 2 台分の `SingleVehicleRuntime` を所有
- global `START` / `STOP` / `RESET` は 2 台へ適用済み
- `VehicleRaceStatus` / `LapEvent` は 2 台分 publish 済み
- race-wide `completed` は `all participating vehicles finished`
- docs は first slice 前提に更新済み

---

## 正本
- 現在進捗: `docs/status/PROJECT_STATUS.md`
- レビュー原則: `docs/process/REVIEW_CONTEXT.md`
- validation: `docs/process/dev_validation.md`
- review checklist: `docs/process/review_checklist.md`
- current demo 仕様: `docs/specs/milestone1.md`

矛盾がある場合は repo docs を優先する。

---

## レビュー原則の要約
- issue 単位で小さく実装
- build が通るだけで OK にしない
- 差分境界・責務分離・仕様一致・validation を重視
- 将来機能の先回り実装は避ける
- CMakeLists.txt の不要差分を許さない

---

## 次に見るべきもの
1. `PROJECT_STATUS.md`
2. 今回の `git diff`
3. 必要なら `milestone1.md` / `dev_validation.md` / `review_checklist.md`
