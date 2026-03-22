# Multi-Vehicle Runtime Model

この文書は、current single-vehicle demo を土台に、future multi-vehicle support に向けた最小 internal runtime model を整理するための設計メモです。

この文書の目的:

- current single-vehicle demo の runtime 構造を短く要約する
- future multi-vehicle support に向けた最小 internal runtime model を整理する
- first multi-vehicle slice で採用する runtime 責務境界を明示する
- Issue #43 の completion semantics と接続しやすい前提を置く
- 後続の実装設計 issue に分解しやすい粒度で論点を固定する

この文書でやらないこと:

- code change
- new node implementation
- `.msg` file change
- dynamic join / leave
- ranking / leaderboard 設計
- collision / overtaking
- planner / control integration
- launch redesign

future multi-vehicle support の内容は、現時点では未実装です。この文書は永久仕様ではなく、first multi-vehicle slice に向けた最小整理です。

## Current Single-Vehicle Runtime Summary

current demo では、`race_progress_publisher` が `SingleVehicleRuntime` を内部に 1 つ持ち、`/race_command` に応じて `start` / `stop` / `reset` を適用しながら、tick ごとに progress を進めます。

`SingleVehicleRuntime` の current な役割は次のとおりです。

- `start` / `stop` / `reset` を受ける
- fixed position 列を 1 step ずつ進める
- `ProgressTracker` を通して 1 台分の lap progress を更新する
- target lap count 到達で `has_finished` と `completed` を成立させる
- position 列終端で完了条件未達なら `stopped` に遷移し得る

single-vehicle では race-wide state と vehicle-local progress が 1 台分に重なっているため、`SingleVehicleRuntime` 1 つで成立しています。

## Why Multi-Vehicle Needs an Explicit Runtime Model

multi-vehicle では、各車両の progress 更新と race 全体の state 集約を同じ runtime に閉じ込めると、責務境界が曖昧になります。

先に internal runtime model を分けておく理由は次のとおりです。

- participating vehicles の集合管理は race-wide 責務になる
- 各車両の lap progress と finish は vehicle-local 責務になる
- race-wide completion は各車両状態の集約として扱う必要がある
- single-vehicle では暗黙だった command scope と tick scope を multi-vehicle では明示する必要がある

## Recommended Shape

推奨構成は `one coordinator + many vehicle runtimes` です。

- race-level coordinator が participating vehicles と race-wide state を持つ
- vehicle ごとに vehicle-local runtime を 1 つ持つ
- current `SingleVehicleRuntime` は vehicle-local runtime の原型として扱う
- coordinator は vehicle runtimes を直接束ね、first slice では全車一括で command と tick を適用する

この構成にしておくと、current single-vehicle demo を大きく飛び越えずに multi-vehicle へ拡張できます。

## Responsibility Boundary

### Coordinator Responsibilities

race-level coordinator の最小責務は次のとおりです。

- participating vehicles の固定集合を保持する
- race-wide operational state を持つ
- global `START` / `STOP` / `RESET` command を受ける
- tick の進行単位を決め、全 vehicle runtime を一括更新する
- vehicle runtime の出力を集約して race-wide completion を判定する
- race-wide state と vehicle-level 出力 publish の起点になる

first slice では coordinator が race 全体の owner になり、車両ごとの command scope は持ち込みません。

### Vehicle Runtime Responsibilities

vehicle-level runtime の最小責務は次のとおりです。

- 1 台分の progress state を保持する
- 1 台分の `start` / `stop` / `reset` 適用対象になる
- tick 1 回分の position / lap progress / finish 状態を更新する
- `VehicleRaceStatus` と `LapEvent` に必要な vehicle-local 情報を生成する
- 自車が finished したかどうかを表す

vehicle runtime は race-wide completion や participating vehicles 集合を持ちません。`SingleVehicleRuntime` が現在すでに持っている責務は、おおむねこの vehicle-local 側に属します。

## Minimal Tick / Update Flow

first multi-vehicle slice の tick flow は、coordinator による全車一括進行を採用します。

1. coordinator が race-wide state を確認する
2. race が `running` のときだけ tick を 1 回進める
3. coordinator が participating vehicles 全件に対して vehicle runtime を 1 回ずつ更新する
4. 各 vehicle runtime が vehicle-local progress update を返す
5. coordinator が全車分の結果を集約して race-wide completion を判定する
6. coordinator が race-wide state と vehicle-level publish 内容を確定する

first slice では、tick cadence は race-wide に 1 つだけ持ち、車両ごとの独立 cadence は扱いません。

## Minimal Command Flow

first multi-vehicle slice の command flow は global command のみとします。

- `START`: coordinator が participating vehicles 全件に対して開始を適用し、race-wide state を `running` にする
- `STOP`: coordinator が participating vehicles 全件に対して停止を適用し、race-wide state を `stopped` にする
- `RESET`: coordinator が participating vehicles 全件を初期化し、race-wide state を初期状態へ戻す

この slice では、特定 vehicle のみを対象とする command は扱いません。command scope は race 全体に固定します。

## Participating Vehicles Policy for the First Slice

first multi-vehicle slice では、participating vehicles は起動時固定とします。

- race 開始前に coordinator が対象 `vehicle_id` 集合を確定する
- race 開始後の追加参加や離脱は扱わない
- completion aggregation の対象集合もこの固定集合に一致させる

この制約により、first slice では dynamic join / leave を持ち込まずに runtime model を単純に保てます。

## Completion Aggregation Connection

Issue #43 の前提に接続するため、first slice では race-wide completion を `all participating vehicles finished` として扱います。

- 各 vehicle runtime は vehicle-local な `has_finished` を持つ
- coordinator は participating vehicles 全件の finished 状態を集約する
- 全 participating vehicles が finished になったときだけ race-wide `completed` を成立させる

single-vehicle では `SingleVehicleRuntime` の completed と vehicle finished が一致して見えますが、multi-vehicle では coordinator が race-wide completion の責務を持つ点が違います。

## Alternatives Not Adopted

### Monolithic Multi-Vehicle Runtime

1 つの巨大 runtime が race-wide state と全車 progress をまとめて持つ案は、今回は採用しません。

採用しない理由:

- race-level と vehicle-level の責務境界が曖昧になりやすい
- current `SingleVehicleRuntime` を vehicle-local 原型として再利用しにくい
- 後続で ranking や vehicle lifecycle を追加すると肥大化しやすい

### Dynamic Join / Leave in the First Slice

first slice から参加車両の途中追加 / 離脱を扱う案は、今回は採用しません。

採用しない理由:

- participating vehicles 集合の確定条件が複雑になる
- completion aggregation の意味が不安定になる
- first slice の最小 runtime model から外れる

## Open Questions

次はまだ未確定です。

- coordinator を既存 `race_progress_publisher` の内部拡張として置くか、別 component として置くか
- vehicle runtime 間で position source や tick 入力をどこまで共有するか
- `RaceState` に race-wide aggregate をどう反映するか
- finished 後の vehicle runtime を以後どう扱うか
- current single-vehicle の `stopped` と multi-vehicle の `completed` を最終的にどの state model で表すか

## Connection to Next Design Work

この文書を前提に、後続 issue では少なくとも次を分けて検討できます。

- coordinator の内部 state と API 境界
- vehicle runtime の最小 interface
- current `race_progress_publisher` へどう内包するか、またはどう分離するか
- `RaceState` と `VehicleRaceStatus` の publish responsibility
- first multi-vehicle slice の実装単位

Issue #46 では、この runtime model を前提に coordinator と vehicle runtime の最小 interface と state ownership を詰めるのが自然です。
