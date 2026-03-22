# Message Responsibility Boundary

この文書は、current single-vehicle demo を前提に、future multi-vehicle support に向けた message responsibility boundary を整理するための設計メモです。

この文書の目的:

- current single-vehicle demo で使っている message の責務境界を短く整理する
- future multi-vehicle support で race-level message と vehicle-level message を分ける理由を明確にする
- `RaceState`、`VehicleRaceStatus`、`LapEvent` の役割を整理する
- 後続 issue で message redesign や runtime 設計を議論しやすい前提を置く

この文書でやらないこと:

- code change
- `.msg` file change
- multi-vehicle 実装
- ranking / leaderboard 設計
- race manager node 実装
- aggregate field の最終 schema 確定

future multi-vehicle support の内容は現時点では未実装です。この文書は current demo を壊さずに将来の責務分離を先に明文化するためのものです。

## Current Single-Vehicle Demo Summary

current demo では、1 台の車両に対して `race_progress_publisher` が次の 3 message を publish します。

- `RaceState`
- `VehicleRaceStatus`
- `LapEvent`

current single-vehicle demo における責務の要約:

- `RaceState`: race-wide phase / state を伝える
- `VehicleRaceStatus`: vehicle-local progress の主情報源
- `LapEvent`: ラップ確定時にだけ流れる event 通知

single-vehicle では race-level と vehicle-level が実質 1 台分に重なるため、一部の値は重複して見えます。特に `RaceState.completed_laps` は、current demo では単一車両の progress を重ねて持つ暫定値です。

## Why Explicit Boundary Is Needed for Multi-Vehicle

future multi-vehicle support では、ある車両は finish 済みだが別の車両はまだ走行中、という状態が自然に発生します。そのため、次を分ける必要があります。

- race-level responsibility: race 全体の phase、state、aggregate
- vehicle-level responsibility: 各車両の lap progress、lap time、finish 状態

この分離が曖昧なままだと、`RaceState` に vehicle-local progress を混在させやすくなり、completion policy、参加車両集合管理、将来の ranking / leaderboard、race manager の責務設計が不安定になります。

## Role of `RaceState`

`RaceState` は race-level message として扱います。

current demo での主な役割:

- race-wide な phase / state を表す
- race 全体の elapsed time を表す
- current single-vehicle demo では `completed_laps` を重複的に持つ

future multi-vehicle support に向けた方針:

- `RaceState` は race-wide phase / aggregate に責務を寄せる
- 各車両の progress の主情報源にはしない
- vehicle ごとの lap 数や lap time を直接運ぶ責務は持たせない

つまり `RaceState` は、race 全体が今どの phase にあるかを伝える message として整理します。

## Role of `VehicleRaceStatus`

`VehicleRaceStatus` は vehicle-level message です。

current demo では次のような vehicle-local progress を持ちます。

- `vehicle_id`
- `lap_count`
- `current_lap_time`
- `last_lap_time`
- `best_lap_time`
- `total_elapsed_time`
- `has_finished`
- `is_off_track`
- `off_track_count`

future multi-vehicle support に向けた方針:

- `VehicleRaceStatus` を vehicle-local progress の主情報源として扱う
- 各車両がどこまで進んだかを読むときは、まず `VehicleRaceStatus` を参照する
- `has_finished` は vehicle-local な finish 状態として扱う

single-vehicle では `RaceState.completed_laps` と `VehicleRaceStatus.lap_count` が同じ進捗を指して見えますが、将来の主情報源は `VehicleRaceStatus` に寄せます。

## Role of `LapEvent`

`LapEvent` は event-only message として扱います。

current demo での役割:

- start line crossing が確定した瞬間を通知する
- lap 完了時の情報を 1 回のイベントとして流す

future multi-vehicle support に向けた方針:

- `LapEvent` は event-only とする
- state carrier にはしない
- 継続状態や最新 progress の主情報源として扱わない

つまり `LapEvent` は「何が起きたか」を伝えるためのものであり、「いま何が真か」を保持する message ではありません。

## `RaceState.completed_laps` in Current Demo and Future Multi-Vehicle

`RaceState.completed_laps` の current single-vehicle demo における意味は、単一車両の `lap_count` を race-level message 側にも重複して持っている、というものです。

current demo ではこの重複を暫定的に許容できます。理由は、参加車両が 1 台だけであり、race-level progress と vehicle-local progress が実質的に一致するためです。

ただし future multi-vehicle support では、この field は redesign candidate として扱います。問題になる点:

- race-wide progress なのか、特定 vehicle の progress なのかが曖昧になる
- 複数車両の lap count を 1 つの scalar で表せない
- aggregate を表したいのか、vehicle-local state を重ねて持ちたいのかが不明確になる

したがって、`RaceState.completed_laps` は current demo では暫定許容、future multi-vehicle では見直し候補と位置づけます。この段階では、廃止、置き換え、aggregate への再定義のいずれにするかは未確定です。

## Forward Direction

future 向けの方針は次のとおりです。

- race-level state は phase / aggregate に寄せる
- vehicle-local progress は `VehicleRaceStatus` を主情報源にする
- `LapEvent` は event-only とする

この方針により、race-wide state と vehicle-local progress を混同せずに multi-vehicle support の議論を進めやすくなります。

## Open Questions

次はこの文書では未確定です。

- `RaceState` にどの aggregate を残すか
- `RaceState.completed_laps` を暫定維持するか、廃止前提で扱うか
- race-wide completion をどの field / schema で表すか
- participating vehicles の管理責務をどこに置くか
- ranking / leaderboard をどの message 層で扱うか

## Connection to Next Issues

この文書を前提に、後続 issue では少なくとも次を分けて検討できます。

- race-level state message に何を残すか
- `RaceState.completed_laps` の redesign 方針
- vehicle-local progress を `VehicleRaceStatus` にどう集約するか
- `LapEvent` を event-only のまま保つために state 表現をどこに置くか
- multi-vehicle support に必要な coordinator / manager 責務

この粒度で責務境界だけを先に明文化しておくことで、Issue #45 以降で message redesign や runtime 設計に接続しやすくします。
