# Race Completion Semantics

この文書は、current single-vehicle demo を土台に、future multi-vehicle support に向けた race-wide completion semantics を整理するための設計メモです。

この文書の目的:

- current single-vehicle demo における completion semantics を短く整理する
- vehicle-local な finish と race-wide な completed を分離して定義する
- first multi-vehicle slice で採用する race-wide completion policy の候補を明示する
- 後続 issue で message / runtime / manager 設計へ接続しやすい前提を置く

この文書でやらないこと:

- code change
- message 定義変更
- multi-vehicle 実装
- ranking / leader 判定
- race manager node 実装
- threshold finisher など高度な競技ルールの確定

future multi-vehicle support の内容は、現時点では未実装です。この文書は将来仕様の永久固定ではなく、first multi-vehicle slice に向けた暫定の整理です。

## Current Single-Vehicle Demo Summary

current demo では 1 台の車両だけが存在し、`SingleVehicleRuntime` が fixed position 列を進めながら progress を更新します。

- `VehicleRaceStatus.has_finished` は、その車両が target lap count に到達したことを表す
- race-wide な `completed` は runtime 全体の完了状態を表す
- 参加車両が 1 台だけなので、実質的には `vehicle finished` と `race completed` が同じタイミングで成立する

一方で、現在の一致は single-vehicle だから成り立っているだけです。multi-vehicle ではこの 2 つを同一意味のまま扱うと責務境界が崩れます。

また current demo では、target lap count 未到達のまま fixed position 列の終端に達した場合、runtime は `stopped` になり得ます。これは `completed` とは別です。

## Why Multi-Vehicle Needs Explicit Semantics

multi-vehicle では、ある車両が finish しても別の車両がまだ走行中、という状態が自然に発生します。そのため、次を明示的に分ける必要があります。

- vehicle-local state: 各車両が finish したか
- race-wide state: race 全体を completed とみなすか

この分離が曖昧なままだと、`RaceState` が vehicle-level progress を抱え込みやすくなり、将来の ranking、DNF、参加車両集合管理、race manager の責務設計が不安定になります。

## Terms

### Vehicle Finished

`vehicle finished` は、ある参加車両が race の finish 条件を満たしたことを表します。first multi-vehicle slice では、最小の意味として「その車両が target lap count に到達した」を想定します。

この状態は vehicle-local です。`VehicleRaceStatus.has_finished` はこの意味を表す field として扱います。

### Race Completed

`race completed` は、race-wide に race 全体の完了条件が成立したことを表します。

これは vehicle-local 状態そのものではなく、参加車両集合に対する集約結果です。したがって、`race completed` は race-wide state の責務であり、個々の `VehicleRaceStatus` の意味と混同しません。

### Stopped

`stopped` は、race が現在進行していないことを表す operational state です。`completed` と同義ではありません。

`stopped` になり得る例:

- 明示的な stop command を受けた
- start 前または reset 直後で race がまだ走っていない
- current single-vehicle demo のように、finish 条件未達のまま runtime が進行を継続できなくなった

`completed` は race の完了条件が成立した状態、`stopped` は進行していない状態であり、両者は別軸です。

## Relationship Between `VehicleRaceStatus.has_finished` and Race-Wide `completed`

first multi-vehicle slice では、次の関係を採用方針とします。

- `VehicleRaceStatus.has_finished` は各車両の finish を表す
- race-wide `completed` は参加車両全体を見た集約結果を表す
- race-wide `completed` は `VehicleRaceStatus.has_finished` を入力として導かれるが、同じ概念ではない

single-vehicle では両者が一致して見えますが、multi-vehicle では一致を前提にしません。

この整理により、vehicle-local progress の主情報源は引き続き `VehicleRaceStatus` に置き、race phase や aggregate 判定は race-wide state 側に残せます。

## Proposed Policy for the First Multi-Vehicle Slice

first multi-vehicle slice では、次を推奨します。

- race-wide `completed` = all participating vehicles finished

ここでいう participating vehicles は、その race に参加するとみなされた vehicle 集合です。集合への参加条件、途中離脱、DNS / DNF の扱いは現時点では未確定です。

この方針は first multi-vehicle slice のための採用候補であり、将来の最終仕様として永久固定するものではありません。

## Why This Policy Is Recommended

- current single-vehicle semantics を最も素直に拡張できる
- ranking や leader 判定が未実装でも定義できる
- race-wide state と vehicle-local state の責務境界を保ちやすい
- completion 判定入力を `has_finished` の集約に限定でき、first slice の実装範囲を増やしにくい
- 「一部車両は finished、race はまだ running」という中間状態を自然に表現できる

特に first slice では、順位や勝者決定を completion semantics に持ち込まないことが重要です。`all participating vehicles finished` は、race completed を winner 判定から切り離せます。

## Alternatives Not Adopted in This Slice

### First Finisher Wins

案:

- 最初の 1 台が finish した時点で race-wide `completed` とする

今回は採用しない理由:

- ranking / leader 判定の意味付けと強く結びつく
- 他車両がまだ走行中でも race completed になり、race-wide state と vehicle progress の関係が分かりにくい
- first slice で決めるには競技ルール依存が強すぎる

### Threshold Number of Finishers

案:

- 規定台数または規定割合の車両が finish した時点で race-wide `completed` とする

今回は採用しない理由:

- 参加台数、失格、DNF、途中離脱の扱いを先に定義する必要がある
- threshold の設定主体が race rule 側に寄り、first slice の責務を超える
- current single-vehicle demo からの自然な拡張になりにくい

### Explicit Stop Only

案:

- vehicle finished は vehicle-local にだけ持ち、race-wide completion は持たず、明示的 stop が来るまで race は running のままにする

今回は採用しない理由:

- race-wide completed という概念自体を先送りしすぎる
- 将来の race manager 設計や race phase 定義に必要な論点が残ったままになる
- finish 済み車両が揃った後の race phase を表す明確な語彙が欠ける

## Open Questions

次はまだ未確定です。

- participating vehicles をいつ、どこで確定するか
- 未開始車両、途中参加、途中離脱をどう扱うか
- DNF / DNS / disqualified のような結果区分を first slice に含めるか
- `RaceState` に race-wide completion をどう表現するか
- current `RaceState.completed_laps` を将来どう扱うか
- `STOPPED` と `COMPLETED` を最終的にどの state model で表現するか

## Connection to Next Design Issues

この文書を前提に、後続 issue では少なくとも次を分けて検討できます。

- race-wide state message に何を持たせるか
- participating vehicles の管理責務をどこに置くか
- race manager / coordinator が completion をどう集約するか
- `RaceState.completed_laps` の整理または廃止方針
- finish 後も他車両が走行中である期間の state transition

先に completion semantics をこの粒度で固定しておくことで、message redesign や multi-vehicle runtime の議論を必要以上に早く結論づけずに進められます。
