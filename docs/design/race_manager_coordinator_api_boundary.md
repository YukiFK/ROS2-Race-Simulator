# Race Manager / Coordinator API Boundary

この文書は、current first multi-vehicle implementation slice を前提に、`race manager` と `RaceCoordinator` の API 境界を小さく整理する design-only メモです。

この文書の目的:

- current first slice における race-wide ownership を明文化する
- `race manager` という語と `RaceCoordinator` の対応関係を整理する
- race manager / coordinator API の follow-up implementation issue に切れる acceptance criteria を固定する

この文書でやらないこと:

- code change
- `.msg` change
- runtime behavior change
- launch change
- 3 台以上への一般化
- ranking / leaderboard / winner determination
- `RaceState.completed_laps` redesign の再議論

## Baseline

この文書は repo 内 docs を正本として、次を前提に置きます。

- current demo は first multi-vehicle implementation slice
- participating vehicles は固定 2 台
- command scope は global `START` / `STOP` / `RESET`
- race-wide completion は `all participating vehicles finished`
- `RaceState` は race-wide state observation
- `VehicleRaceStatus` は vehicle-local progress の主情報源
- `LapEvent` は event-only message
- coordinator は current runtime model 側で race-wide completion aggregation の owner

## Terminology Decision

### `RaceCoordinator`

`RaceCoordinator` は、current first slice の code に存在する **具体 implementation 名** として使います。

current slice で `RaceCoordinator` が表すもの:

- 固定 participating vehicles 集合を持つ runtime-side owner
- global command を全 participating vehicle runtime に適用する owner
- race-wide completion aggregation を行う owner
- race-wide status 判定の source owner

### `race manager`

`race manager` は、この issue では **抽象責務名** としてだけ使います。

この issue での扱い:

- `race manager` は current first slice で新しい node / class / API 名を導入する意味ではない
- current first slice では、race manager 的な最小責務は `RaceCoordinator` が担っていると整理する
- future docs で current implementation を指すときは、曖昧な `race manager` より `RaceCoordinator` を優先して書く
- `race manager` は、将来 `RaceCoordinator` を含むより広い orchestration boundary を議論するときだけ使う

したがって、current first slice の文脈では **`race manager` = `RaceCoordinator` が担う race-wide orchestration responsibility の抽象名** と読むのが推奨です。

## Current First Slice Boundary

### 1. Participating Vehicles Set Ownership

owner:

- `RaceCoordinator`

current ownership:

- fixed 2-vehicle participating set を保持する
- completion aggregation の対象集合をこの fixed set に一致させる
- global command の適用対象集合をこの fixed set に一致させる

not owned here:

- dynamic join / leave
- 3 台以上への一般化
- entry list / grid / result semantics

### 2. Global Command Ingress And Application

ingress owner:

- `race_progress_publisher`

application policy owner:

- `RaceCoordinator`

vehicle-local application target:

- 各 `SingleVehicleRuntime`

current boundary:

- `/race_command` の ROS subscription 入口は `race_progress_publisher` が持つ
- command を race-wide policy としてどう適用するかは `RaceCoordinator` が持つ
- 実際の `start` / `stop` / `reset` の vehicle-local 適用先は各 runtime

このため、current first slice では `RaceCoordinator` は command transport endpoint ではなく、**global command application boundary** として扱う。

### 3. Race-Wide Completion Aggregation

owner:

- `RaceCoordinator`

current ownership:

- 各 participating vehicle の `has_finished` を集約する
- `all participating vehicles finished` を race-wide completion 条件として評価する
- race-wide status を `running` / `completed` / `stopped` の current semantics で返す

not owned here:

- winner determination
- ranking / leaderboard
- DNF / DNS / disqualification
- threshold finisher policy

### 4. Race-Wide State Publish Ownership

state decision owner:

- `RaceCoordinator`

ROS publish owner:

- `race_progress_publisher`

current boundary:

- race-wide state の source decision は `RaceCoordinator` が持つ
- `/race_state` への actual publish は `race_progress_publisher` が持つ
- current first slice では `RaceCoordinator` 自体を ROS publisher node とみなさない

この issue では、race-wide state publish ownership は **state meaning / source ownership** と **ROS transport ownership** を分けて書く。

### 5. Vehicle-Level Publish Orchestration Ownership

owner:

- `race_progress_publisher`

current ownership:

- timer tick の publish flow を進める
- 各 vehicle runtime の tick 結果から `VehicleRaceStatus` / `LapEvent` publish を起動する
- `RaceCoordinator` が持つ participating vehicle 集合と race-wide ownership 境界を前提に publish flow を組み立てる

not owned by `RaceCoordinator` in current slice:

- `VehicleRaceStatus` / `LapEvent` の actual ROS publish
- assembler 呼び出し
- topic transport lifecycle

この整理により、`RaceCoordinator` は race-wide owner だが、vehicle-level topic publish orchestration まで単独所有しているとは書かない。

## Responsibility Split For Current `RaceCoordinator`

### `RaceCoordinator` が持つ責務

- fixed participating vehicles 集合の ownership
- global `START` / `STOP` / `RESET` の race-wide application policy
- all participating vehicles finished の completion aggregation
- current race-wide status の source ownership
- race-wide step / primary snapshot の current runtime-side exposure
- participating vehicle runtime への lookup boundary

### 今は `RaceCoordinator` に持たせない責務

- `/race_command` subscriber そのもの
- `/race_state`、`/vehicle_race_status`、`/lap_event` の actual ROS publish
- assembler 実行
- launch composition
- ranking / leaderboard / winner determination
- dynamic vehicle lifecycle
- result model
- `RaceState.completed_laps` redesign

## API Boundary Guidance For Follow-Up Work

follow-up implementation issue では、次の方向で boundary を維持する。

1. participating vehicles 集合の source of truth は current first slice では `RaceCoordinator` に残す。
2. global command の ROS ingress と race-wide application policy を同一責務として曖昧化しない。
3. race-wide completion aggregation は `RaceCoordinator` 側に残し、vehicle runtime や publisher 側へ分散させない。
4. `RaceState` の race-wide meaning と `/race_state` publish transport を同一責務として混ぜない。
5. `VehicleRaceStatus` / `LapEvent` の vehicle-level publish orchestration を追加整理する場合でも、ranking や result design を同じ issue に混ぜない。

## Acceptance Criteria For A Follow-Up Implementation Issue

1. current first slice docs において、fixed participating vehicles 集合の owner が `RaceCoordinator` であることが一貫して説明されている。
2. global `START` / `STOP` / `RESET` について、ROS command ingress と race-wide application responsibility が分けて説明されている。
3. race-wide completion aggregation の owner が `RaceCoordinator` であり、条件が `all participating vehicles finished` であることが docs と実装方針で一貫している。
4. race-wide state publish について、state source ownership と ROS publish ownership が区別されている。
5. vehicle-level publish orchestration について、`RaceCoordinator` の責務と `race_progress_publisher` の責務が混在していない。
6. current first slice を指す文脈では、抽象語 `race manager` だけで実装主体をぼかさず、必要に応じて `RaceCoordinator` を明示している。
7. code change、`.msg` change、runtime behavior change、launch redesign、3 台以上への一般化、ranking / leaderboard 論点を同じ issue に混ぜない。

## Unresolved / Deferred

この文書では次を未確認または deferred のまま残します。

- 将来 `race manager` を独立 node 名として採用するか
- `RaceCoordinator` を既存 publisher 内部の component として保つか
- publish orchestration を将来 coordinator 側へ寄せるか
- 3 台以上対応時の participating vehicles API shape
- ranking / result model と race-wide completion の関係
