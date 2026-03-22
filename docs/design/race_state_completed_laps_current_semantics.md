# RaceState.completed_laps Current Semantics

この文書は、current demo である first multi-vehicle implementation slice における `RaceState.completed_laps` の **current semantics** を明確化するための小さな設計メモです。

この文書の目的:

- current implementation における `RaceState.completed_laps` の読み方を固定する
- first multi-vehicle slice でこの field が曖昧になりうる理由を明示する
- race-wide completion と vehicle-local finish をどの観測点で判断すべきかを整理する
- 将来の redesign issue に送る open questions を切り分ける

この文書でやらないこと:

- code change
- `.msg` file change
- runtime behavior change
- `RaceState` schema redesign
- `RaceState.completed_laps` の最終意味の確定

## Scope And Current Baseline

この文書は repo の current state にのみ合わせます。

- current demo は first multi-vehicle implementation slice
- participating vehicles は固定 2 台
  - `demo_vehicle_1`
  - `demo_vehicle_2`
- race-wide completion は `all participating vehicles finished`
- `RaceState.completed_laps` は現状では race-wide aggregate ではなく primary vehicle snapshot 由来

## Current Semantics

current implementation では、`RaceState.completed_laps` は publish される `RaceState` の中で primary vehicle snapshot の `lap_count` を表しています。

したがって、first multi-vehicle slice における current semantics は次のとおりです。

- `RaceState.completed_laps` は race 全体の集約値ではない
- `RaceState.completed_laps` は participating vehicles 全件の進捗要約ではない
- `RaceState.completed_laps` は current publish path における primary vehicle snapshot の `lap_count` を表す

この field は current demo で publish され続けていますが、race-wide completion 判定の正本ではありません。

## Why It Becomes Ambiguous In The First Slice

single-vehicle では、race-wide state と vehicle-local progress が 1 台分に重なって見えるため、`RaceState.completed_laps` を lap progress のように読んでも大きなズレが出にくい状態でした。

first multi-vehicle slice では 2 台が同じ race に参加し、片方だけ先に finish する期間が自然に発生します。このとき、1 つの scalar である `RaceState.completed_laps` は少なくとも次を区別できません。

- 2 台全体の race-wide aggregate
- どちらか一方の vehicle-local lap count
- race completion 判定に使うべき進捗要約

そのため、field 名だけを見ると race-wide aggregate のように読めてしまいますが、current implementation はその意味を提供していません。

## How To Read It Now

現時点では、`RaceState.completed_laps` は次のように読むべきです。

- `RaceState` に残っている current snapshot-oriented field の 1 つ
- primary vehicle snapshot に由来する補助的な数値
- first multi-vehicle slice では race-wide completion 判定の根拠にしない値

言い換えると、この field は「race 全体が何周完了したか」を表す値としてではなく、current publish path に残っている snapshot 的な値として扱います。

## How Not To Read It

現時点では、`RaceState.completed_laps` を次のように読んではいけません。

- participating vehicles 2 台の race-wide aggregate lap count
- race 全体としての完了度を直接表す値
- `race_state status=completed` と 1 対 1 に対応する値
- どの車両が finish したかを判断するための主観測値
- current multi-vehicle semantics が将来もこのまま固定されることを示す値

特に、`completed_laps=2` のような値だけを見て「race-wide に completed した」と判断してはいけません。

## What To Look At Instead

### Race-Wide Completion

race-wide completion を判断したいときは `race_state status` を見ます。

- `race_state status=completed`
- 意味: `all participating vehicles finished`

### Vehicle-Local Finish

各車両が finish したかを判断したいときは `VehicleRaceStatus.has_finished` を見ます。

- `vehicle_race_status vehicle_id=demo_vehicle_1 ... has_finished=true`
- `vehicle_race_status vehicle_id=demo_vehicle_2 ... has_finished=true`
- 意味: その車両が vehicle-local に finish した

### Practical Reading Order

current demo の観測では、次の読み分けを使います。

- race-wide state は `RaceState.race_status`
- vehicle-local progress は `VehicleRaceStatus`
- lap crossing event は `LapEvent`
- `RaceState.completed_laps` は補助的な snapshot としてのみ扱う

## Open Questions For A Future Redesign Issue

この文書では次を確定しません。将来の redesign issue に送る open questions として残します。

- `RaceState.completed_laps` を将来も維持するか
- 維持する場合、race-wide aggregate として再定義するか
- 維持する場合、別名や別 field への分離が必要か
- `RaceState` に race-wide progress 要約を残すべきか
- race-wide aggregate を残すとして、何を aggregate と呼ぶべきか
- `RaceState` と `VehicleRaceStatus` の責務境界をどこまで schema 上でも明示するか

重要なのは、これらが **未決定** だという点です。この文書は current semantics の読み方を固定するものであり、最終 schema を決めるものではありません。
