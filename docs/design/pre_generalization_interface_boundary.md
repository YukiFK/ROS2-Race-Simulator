# Pre-Generalization Interface Boundary

この文書は、current first multi-vehicle implementation slice を前提に、3 台以上 / generalization implementation に進む前の **interface stop line** を小さく固定する design-only メモです。

この文書の目的:

- first slice の実装で既に置かれている interface boundary のうち、generalization 前に stable とみなすものを明示する
- まだ current first slice 専用とみなす interface を切り分ける
- deferred のまま残す論点を implementation issue へ持ち込まないようにする
- 3 台以上 / generalization implementation issue に切るための acceptance criteria を固定する

この文書でやらないこと:

- code change
- `.msg` change
- runtime behavior change
- launch change
- 3 台以上 implementation
- dynamic join / leave
- ranking / leaderboard / winner determination
- `RaceState.completed_laps` の再整理

## Baseline

この文書は repo 内 docs を正本として、次を前提に置きます。

- current demo は first multi-vehicle implementation slice
- participating vehicles は固定 2 台
- command scope は global `START` / `STOP` / `RESET`
- `RaceState` は race-wide state observation
- `VehicleRaceStatus` は vehicle-local progress の主観測
- `LapEvent` は event-only
- race-wide completion は `all participating vehicles finished`
- `RaceCoordinator` / publisher ownership boundary は `docs/design/race_manager_coordinator_api_boundary.md` で整理済み
- `RaceState.completed_laps` の current semantics と future handling direction は別 design doc で整理済み

## Decision Goal

今回固定したいのは、3 台以上 / generalization implementation issue が **どこを前提に進んでよく、どこから先を別論点として止めるか** です。

今回確定しないこと:

- generalized runtime API の具体的な class / method shape
- participating vehicle set の構成入力方法
- `RaceState.completed_laps` の最終方針
- per-vehicle command の導入可否
- dynamic vehicle lifecycle
- ranking / result model

## Classification

### Stable Before Generalization

generalization issue に進む前でも、次は interface boundary として stable 扱いにします。

#### 1. Participating Vehicle Set Handling

- participating vehicle set の source of truth は `RaceCoordinator` 側に置く
- global command の適用対象集合と completion aggregation の対象集合は同じ participating vehicle set に一致させる
- participating vehicle set を race-wide responsibility として扱い、vehicle-local runtime 側に分散させない

#### 2. Coordinator-Facing Runtime Access Shape

- runtime model は `one coordinator + per-vehicle runtime` の形を維持する
- `RaceCoordinator` は participating vehicles に対応する vehicle runtime 群へアクセスする race-wide owner として扱う
- vehicle runtime は vehicle-local progress と `has_finished` を返す側に留め、race-wide completion owner にしない

#### 3. Race-Wide State Source Shape

- race-wide state meaning の source owner は `RaceCoordinator` に置く
- race-wide completion は coordinator が participating vehicles 全件の vehicle-local finish を集約して決める
- `RaceState` は race-wide state observation として扱い、vehicle-local progress の主観測に戻さない
- vehicle-local progress の主観測は `VehicleRaceStatus`、event 観測は `LapEvent` とする

#### 4. Publisher-Facing Coordinator Interface

- `/race_command` の ROS ingress は publisher 側が持つ
- `/race_state`、`/vehicle_race_status`、`/lap_event` の actual ROS publish も publisher 側が持つ
- `RaceCoordinator` は race-wide meaning、command application policy、completion aggregation、participating runtime lookup の owner として扱う
- state meaning / source ownership と ROS transport ownership を混ぜない

#### 5. Command Scope

- command scope は race-wide global `START` / `STOP` / `RESET` を前提にする
- generalization issue では command の対象台数を広げても、command scope 自体を per-vehicle に変えない
- command transport と command application policy を同一責務として曖昧化しない

### First-Slice-Only For Now

次は current first slice の成立条件としては受け入れるが、generalization 前の stable interface とはみなさない。

#### 1. Participating Vehicle Set Handling

- participating vehicles が `demo_vehicle_1` と `demo_vehicle_2` の 2 台固定であること
- `RaceCoordinator` が 2 台分の `SingleVehicleRuntime` を直接所有している current cardinality

#### 2. Coordinator-Facing Runtime Access Shape

- current code での primary runtime / secondary runtime のような slot-oriented な持ち方
- generalization 前の実装都合としての 2 台前提 lookup

#### 3. Race-Wide State Source Shape

- current `RaceState.completed_laps` が primary vehicle snapshot 由来であること
- race-wide state publish path が current first slice の snapshot exposure を含んでいること

#### 4. Publisher-Facing Coordinator Interface

- current first slice の内部 wiring に依存した coordinator 呼び出し順や assembler 接続
- publisher が current fixed 2-vehicle flow に最適化されたままの内部手順

#### 5. Command Scope

- command 適用先が current fixed 2-vehicle set であること
- generalization 前の current demo validation が 2 台固定シナリオだけを扱うこと

### Explicitly Deferred

次はこの issue では deferred のまま残し、generalization implementation issue にも自動では含めません。

#### 1. Participating Vehicle Set Handling

- 3 台以上での config schema や launch 引数 shape
- dynamic join / leave
- entry list / grid / start order / result semantics

#### 2. Coordinator-Facing Runtime Access Shape

- runtime factory、lifecycle orchestration、runtime replacement の扱い
- finished vehicle を completion 後にどう保持 / 停止 / 再利用するか
- coordinator を既存 publisher 内部 component のまま保つかどうか

#### 3. Race-Wide State Source Shape

- `RaceState.completed_laps` の remove / rename / aggregate redesign
- race-wide aggregate progress が必要かどうか
- ranking / leaderboard / winner determination を race-wide state にどう反映するか

#### 4. Publisher-Facing Coordinator Interface

- coordinator 自体が ROS publisher / subscriber ownership を持つかどうか
- publish orchestration を coordinator 側へ寄せるかどうか
- new node / component decomposition

#### 5. Command Scope

- per-vehicle command
- partial stop / staggered start / command targeting
- richer race control commands

## Guidance For The Generalization Stop Line

3 台以上 / generalization implementation issue は、次の線を越えない限り current boundary に整合します。

- 参加台数を fixed 2 から広げても、participating vehicle set ownership は `RaceCoordinator` に残す
- runtime cardinality を広げても、race-wide completion aggregation owner は `RaceCoordinator` に残す
- race-wide state と vehicle-local progress の観測責務を入れ替えない
- publisher と coordinator の ownership split を崩さない
- command scope を global `START` / `STOP` / `RESET` の外へ広げない

逆に、次が必要になった時点で generalization issue ではなく別 design / implementation issue に切るべきです。

- dynamic join / leave を導入したい
- per-vehicle command を導入したい
- `RaceState` schema や `.msg` を変えたい
- ranking / leaderboard / winner determination を completion semantics に混ぜたい
- publisher / coordinator ownership を再分配したい

## Acceptance Criteria For A 3+ / Generalization Implementation Issue

1. participating vehicle set を 2 台固定から一般化しても、source of truth、command application target set、completion aggregation target set が `RaceCoordinator` 側で一貫している。
2. generalization 後も runtime model が `one coordinator + per-vehicle runtime` の責務分離を保ち、vehicle runtime 側へ race-wide completion ownership を移していない。
3. race-wide state source shape は維持され、`RaceState` を race-wide observation、`VehicleRaceStatus` を vehicle-local progress の主観測、`LapEvent` を event-only とする説明が崩れていない。
4. publisher-facing boundary は維持され、ROS command ingress / topic publish transport と coordinator の race-wide meaning / aggregation ownership が混在していない。
5. command scope は global `START` / `STOP` / `RESET` のままであり、per-vehicle command や richer control policy を同じ issue に持ち込んでいない。
6. `RaceState.completed_laps` を race-wide aggregate として再解釈する新規仕様や `.msg` change を同じ issue に含めていない。
7. dynamic join / leave、ranking / leaderboard、winner determination、launch redesign、runtime behavior change を同じ issue に混ぜていない。
8. docs / validation の記述が、generalized participating vehicle count に関する変更点だけを反映し、first-slice-only と deferred の境界を崩していない。

## Relationship To Existing Design Docs

- `docs/design/first_multi_vehicle_slice.md`:
  - first slice の実装 stop line を定義する文書
- `docs/design/multi_vehicle_runtime_model.md`:
  - `one coordinator + per-vehicle runtime` の責務分離を定義する文書
- `docs/design/message_boundary.md`:
  - `RaceState`、`VehicleRaceStatus`、`LapEvent` の観測責務を定義する文書
- `docs/design/race_manager_coordinator_api_boundary.md`:
  - coordinator と publisher の ownership split を定義する文書
- `docs/design/race_state_completed_laps_future_handling.md`:
  - `RaceState.completed_laps` を generalization issue に持ち込まないための future direction を定義する文書

この文書は、それらの design doc を置き換えるものではなく、**generalization implementation 前に固定しておく interface classification** を横断的に要約するものです。
