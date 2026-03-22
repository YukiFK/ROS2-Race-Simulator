# First Multi-Vehicle Implementation Slice

この文書は、current single-vehicle demo を前提に、future multi-vehicle support の first implementation slice を小さく固定するための設計メモです。

この文書の目的:

- first multi-vehicle implementation slice の scope を固定する
- success criteria を観測可能な形で明示する
- explicit out-of-scope を定義する
- 後続の implementation issue に切り出しやすい stop line を置く
- Issue #43 の completion semantics、Issue #44 の message boundary、Issue #45 の runtime model と矛盾しない前提をまとめる

この文書でやらないこと:

- code change
- `.msg` file change
- ranking / leaderboard implementation
- collision / overtaking
- planner / control integration
- dynamic join / leave
- GUI / Foxglove optimization
- race result table design

future multi-vehicle support の内容は現時点では未実装です。この文書は first slice を intentionally small に保つための暫定整理であり、将来仕様の永久固定ではありません。

## Why This Slice Is Needed

current demo は single-vehicle 前提で成立していますが、multi-vehicle に進む前に次を先に固定しておく必要があります。

- 何を first delivery とみなすか
- race-wide state と vehicle-local progress をどこまで実装対象に含めるか
- completion semantics をどこまで observable に確認できれば十分か
- 後続へ送る論点をどこで打ち切るか

この stop line がないまま実装に入ると、ranking、result 表現、vehicle lifecycle、message redesign まで scope が広がりやすくなります。first slice では「2 台を同じ race で一緒に進め、global command で制御し、race-wide state と per-vehicle status を publish できる」ところで止めます。

## Assumptions

first slice は次の前提を置きます。

- current single-vehicle demo を出発点にする
- participating vehicles は 2 台固定を推奨案とする
- 2 台は shared track を走る
- progression は fixed または current demo に近い deterministic progression を使う
- command scope は global `START` / `STOP` / `RESET` のみとする
- vehicle-local progress の主情報源は `VehicleRaceStatus` とする
- lap crossing の event 通知は `LapEvent` を使う
- race-wide state は vehicle-local progress と分離して扱う
- race-wide completion は Issue #43 に合わせて `all participating vehicles finished` を採用する
- first slice では future multi-vehicle support の全体像ではなく、最小の implementation slice だけを対象にする

## Recommended Scope

first multi-vehicle implementation slice の推奨 scope は次のとおりです。

### 1. Two Fixed Participating Vehicles

- participating vehicles は 2 台固定
- `vehicle_id` 集合は race 開始前に固定
- race 開始後の join / leave は扱わない

### 2. Shared Track and Fixed Demo Progression

- 2 台は同じ track model を共有する
- progression source は fixed path、fixed positions、または同等の demo progression に限定する
- first slice では高度な path planning や vehicle-specific control loop は持ち込まない

### 3. Global Command Scope Only

- `START` は 2 台の race progression を一括で開始する
- `STOP` は 2 台の race progression を一括で停止する
- `RESET` は 2 台の progress と race-wide state を一括で初期化する
- 特定車両だけを対象にする command は扱わない

### 4. Per-Vehicle Status Publish

- 各 participating vehicle について `VehicleRaceStatus` を publish できる
- 各車両の `vehicle_id`、lap progress、finish 状態は車両単位で観測できる
- `has_finished` は vehicle-local completion を表す

### 5. Per-Vehicle Lap Event Publish

- 各 participating vehicle について `LapEvent` を publish できる
- lap crossing は vehicle 単位の event として観測できる
- `LapEvent` は event-only とし、継続状態の主情報源にはしない

### 6. Race-Wide State Publish

- race 全体の operational state を publish できる
- race-wide `completed` は participating vehicles 全件の `has_finished` 集約として扱う
- race-wide state は vehicle-local lap progress の主情報源にはしない

## Observable Success Criteria

first slice の success criteria は、少なくとも次を外部観測で確認できることです。

1. 2 台の固定 participating vehicles が同じ race session に含まれて開始される。
2. `START` 後、2 台それぞれの `VehicleRaceStatus` が車両単位で更新される。
3. 2 台それぞれの lap crossing に対して、対応する `LapEvent` が車両単位で publish される。
4. 片方の車両だけが先に `has_finished=true` になっても、もう片方が未完了である間は race-wide completion は成立しない。
5. 両方の participating vehicles が `has_finished=true` になった後にだけ、race-wide `completed` が成立する。
6. `STOP` は race 全体に効き、両車両の progression が停止する。
7. `RESET` は race 全体に効き、両車両の progress、finish 状態、race-wide state が初期状態に戻る。
8. 観測できる責務境界として、vehicle-local progress は `VehicleRaceStatus` と `LapEvent`、race-wide state は race-level publish に分かれている。

## Explicit Out-of-Scope

first slice では次を明示的に扱いません。

- 3 台以上への一般化
- dynamic join / leave
- ranking / leaderboard
- winner 決定ルール
- collision / overtaking
- vehicle 間の相互干渉モデル
- planner / control integration
- GUI / Foxglove 向けの見せ方最適化
- result summary や race result table
- DNF / DNS / disqualification
- per-vehicle command scope
- `.msg` schema redesign
- `RaceState.completed_laps` の最終方針確定

## Stop Line for Initial Delivery

初回実装完了は、次を満たした時点とみなします。

- 2 台固定の shared-track demo が成立している
- global `START` / `STOP` / `RESET` で race 全体を制御できる
- 各車両の `VehicleRaceStatus` と `LapEvent` を publish できる
- race-wide state を publish できる
- race-wide completion が `all participating vehicles finished` として観測できる
- single-vehicle 前提の責務混在を広げずに、race-wide state と vehicle-local progress の分離が維持されている

逆に、次が必要になった時点で first slice の stop line を越えています。

- ranking や winner 判定を求める
- vehicle lifecycle を可変にしたくなる
- message schema の redesign を始める
- collision、overtaking、planner integration を持ち込みたくなる
- result table や UI 最適化まで含めたくなる

## Follow-Up Work

first slice の後続で扱う項目は次を想定します。

- 参加車両数の一般化
- coordinator / manager の内部 interface 整理
- `RaceState` の aggregate 表現見直し
- ranking / leaderboard と result model
- DNF / DNS / disqualification の定義
- per-vehicle command scope の要否判断
- dynamic join / leave の lifecycle 設計
- collision / overtaking と track occupancy の扱い
- GUI / visualization 向けの情報整理

## Next Steps for Implementation Issues

後続の implementation issue は、少なくとも次の単位に分けやすいです。

1. 2 台固定の participating vehicle 設定と coordinator ownership を実装する。
2. current single-vehicle runtime を vehicle-local runtime として 2 台分扱える形に整理する。
3. global `START` / `STOP` / `RESET` を 2 台へ一括適用する command flow を実装する。
4. per-vehicle `VehicleRaceStatus` publish を 2 台分で観測できるようにする。
5. per-vehicle `LapEvent` publish を 2 台分で観測できるようにする。
6. participating vehicles 全件の `has_finished` 集約として race-wide completion を publish する。
7. first slice の success criteria を確認する demo / validation 手順を追加する。

この粒度で止めておくことで、first multi-vehicle implementation slice は intentionally small に保たれ、Issue #43、#44、#45 と整合したまま実装 issue に分解できます。
