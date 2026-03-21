# Multi-Vehicle Design Memo

この文書は current single-vehicle demo を土台に、将来 multi-vehicle 対応へ進む前に state / message / runtime の責務境界を整理するための設計メモです。

この文書で確定しないこと:

- message 定義の変更
- multi-vehicle 実装
- ranking / leader 判定
- race manager node 実装
- GUI / Foxglove 対応

実装済みの範囲を超える内容は、すべて候補または未確定事項として扱います。

## Current Single-Vehicle Structure

current demo は次の 3 要素で構成されています。

- `race_progress_publisher`: `/race_command` を受けて `/race_state`、`/vehicle_race_status`、`/lap_event` を publish する
- `race_progress_monitor`: 上記 topic を subscribe して表示する
- `race_command_cli`: `start` / `stop` / `reset` を `/race_command` に publish する

現在の runtime は `SingleVehicleRuntime` が中心で、固定 position 列を 1 秒ごとに進めながら 1 台分の progress を更新します。

現在の責務境界:

- `RaceState`: race-level の実行状態を表す
- `VehicleRaceStatus`: 車両単位 progress の主情報源
- `LapEvent`: ラップ確定時のイベント通知

current single-vehicle demo では `RaceState.completed_laps` が単一車両の `lap_count` を重ねて持っていますが、これは multi-vehicle を前提にした race-wide progress ではありません。

## Multi-Vehicle 化で増える責務

single-vehicle では 1 つの runtime に閉じていた責務が、multi-vehicle では少なくとも次の 2 層に分かれます。

- race-level 責務: race 全体の実行状態、参加車両集合、開始 / 停止 / 完了の扱い
- vehicle-level 責務: 各車両の lap progress、finish 状態、off-track 状態、個別イベント

この分離を先に明確にしておかないと、single-vehicle では便利だった重複値が multi-vehicle で曖昧になります。特に race-wide state と vehicle-local progress を同じ message に混在させると、completion policy や将来の ranking 実装の責務が崩れやすくなります。

## Race-Level State の候補

multi-vehicle で race-level に置く候補は次のとおりです。

- race 実行状態: `stopped` / `running` / `completed` のような race 全体状態
- race 設定の参照: target lap 数など、全車共通で扱う設定値
- 参加車両集合: どの `vehicle_id` が race に参加中か
- race-wide completion の結果: race 全体が完了したかどうか
- 開始 / 停止 / reset の適用対象: 全車一括で扱うのか、一部だけ許すのかという policy

race-level に置かないほうがよいもの:

- 各車両の現在 lap 数そのもの
- 各車両の current lap time / best lap time
- 各車両ごとの off-track 回数

これらは vehicle-level progress の責務として持たせたほうが境界が明確です。

## Vehicle-Level State の候補

各車両に閉じた state として持つ候補は次のとおりです。

- `vehicle_id`
- lap 数
- current / last / best lap time
- total elapsed time
- `has_finished`
- `is_off_track`
- off-track 回数
- ラップ crossing のイベント情報

将来的に必要になりそうだが、まだ未確定なもの:

- grid 順、スタート順、entry 情報
- DNF / DNS のような race result 区分
- penalty や invalid lap の扱い
- vehicle ごとの control 状態や sensor 状態

## Race-Wide State と Vehicle-Local Progress の責務分離

責務境界は次のように置くのが最小です。

- race-wide state は「race 全体が今どういう phase にあるか」を表す
- vehicle-local progress は「各車両がどこまで進んだか」を表す
- race 完了判定は race-level の責務だが、その入力は vehicle-level progress から集約する

この前提では、`RaceState` は aggregate された結果や phase を扱い、各車両の progress の主情報源にはしません。

single-vehicle の `RaceState.completed_laps` は multi-vehicle では特に曖昧になりやすい値です。将来の候補としては次のいずれかになります。

- race-level message から lap progress 系を外す
- race-wide aggregate と vehicle-local lap count を別表現に分ける
- race-level には completion 判定に必要な最小限の要約だけ残す

ただし、どの field 名や message 構成にするかは現時点では未確定です。

## Current Message の流用可能性と見直し候補

そのまま流用しやすいもの:

- `RaceCommand`: `start` / `stop` / `reset` の race-wide 制御という意味では当面そのまま使いやすい
- `VehicleRaceStatus`: vehicle-local progress の主情報源という方向性はそのまま使いやすい
- `LapEvent`: vehicle ごとの lap 確定イベントとしてはそのまま拡張しやすい

将来の見直し候補:

- `RaceState.completed_laps` のような single-vehicle 前提の重複 progress
- `RaceState` が race-wide state と progress 要約を同時に持つ構成
- completion policy 確定後に必要になる race-wide summary 表現
- command の適用範囲を race 全体だけに固定するかどうか

重要なのは、現時点では message redesign の必要性をメモするだけで、名前や構成をまだ確定しないことです。

## Runtime 構成の最小案

実装前提を増やしすぎない最小案は、race-level coordinator と vehicle-level runtime を分ける構成です。

- vehicle-level runtime: 各車両の progress 更新を担当する
- race-level coordinator: command 受付、全体 state、completion 判定の集約を担当する

最小イメージ:

- 現在の `SingleVehicleRuntime` は将来の vehicle-local runtime の原型として扱う
- multi-vehicle では vehicle ごとに独立した progress 更新単位を持つ
- race-level 側はそれらを束ねて `/race_state` を決める
- `/vehicle_race_status` と `/lap_event` は vehicle ごとに publish される

この段階では、race-level coordinator を新規 node にするか、既存 publisher を拡張するかは未確定です。

## Completion Policy の候補

race completion policy はまだ確定しません。候補だけ挙げます。

- 先頭 1 台が target lap 到達した時点で race completed とする
- 全車が target lap 到達した時点で race completed とする
- 規定台数が finish した時点で race completed とする
- 明示的 stop が入るまで race は running のままにして、finish は vehicle ごとにだけ持つ

未確定事項:

- `VehicleRaceStatus.has_finished` と race-level `completed` の関係
- finish した車両と running 中の車両が混在する期間の扱い
- 位置情報や順位未実装の段階で何を completion 入力に使うか

## 実装前に先に決めるべきこと

- race-level state に何を残し、何を vehicle-level に寄せるか
- `RaceState.completed_laps` を暫定維持するか、将来廃止前提で扱うか
- `target_lap_count` を race-wide 設定としてどこに置くか
- completion policy をどのレベルまで先に固定するか
- multi-vehicle 最小実装で必要な vehicle 集合の管理単位

## 今後の実装ステップ案

1. race-level state と vehicle-level progress の責務境界を issue として固定する
2. current message を維持したまま multi-vehicle runtime の最小構成を決める
3. `RaceState` の single-vehicle 依存値をどう扱うかを別 issue で整理する
4. completion policy を候補から 1 つ以上に絞り、未確定点を明示する
5. その後に message change の要否を判断する
6. 最後に runtime / node 構成の実装 issue へ分解する

この順序にしておくと、message redesign を先走って確定せず、current demo を壊さないまま multi-vehicle 化の論点を分離できます。
