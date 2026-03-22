# RaceState.completed_laps Future Handling

この文書は、current demo である first multi-vehicle implementation slice を前提に、`RaceState.completed_laps` の **future handling** を小さく比較整理するための design-only メモです。

この文書の目的:

- `RaceState.completed_laps` の current semantics を変更せずに、future handling の選択肢を比較する
- race-wide responsibility と vehicle-local responsibility の分離に照らして推奨方針を 1 つ選ぶ
- 後続 implementation issue に渡す acceptance criteria と deferred work を固定する

この文書でやらないこと:

- code change
- `.msg` file change
- runtime behavior change
- API redesign
- coordinator / manager behavior change
- 3 台以上への一般化
- ranking / leaderboard / winner determination

## Baseline

この文書は repo の current docs を正本として、次を前提に置きます。

- current demo は first multi-vehicle implementation slice
- `RaceState` は race-wide state observation
- `VehicleRaceStatus` は vehicle-local progress の主情報源
- `LapEvent` は event-only message
- current `RaceState.completed_laps` は race-wide aggregate ではなく primary vehicle snapshot 由来
- current semantics は `docs/design/race_state_completed_laps_current_semantics.md` で明文化済み
- future handling は未決定

## Decision Goal

今回決めたいのは、`RaceState.completed_laps` を future multi-vehicle support に向けて **どの責務方向へ寄せるべきか** です。

今回決めないこと:

- field の rename / removal の実施可否
- message schema の最終形
- publish 内容の即時変更
- consumer migration の具体的手順

## Options

### Option A. Race-Wide Aggregate に再定義する

`RaceState.completed_laps` を race-wide aggregate field として再定義する案です。

考えられる読み方の例:

- all participating vehicles のうち最小 lap 数
- 全車の合計 lap 数
- race-wide completion に対応する何らかの aggregate progress

ただし、どの aggregate を意味するかは current docs では未確定です。

評価:

- race-wide responsibility と vehicle-local responsibility の分離:
  - field を race-wide 側に寄せる点では方向は合う
  - ただし scalar 1 つで何の aggregate かを追加説明なしに読めないため、曖昧さが残りやすい
- current first slice docs との整合:
  - current docs は「aggregate ではない」と明記しているため、将来方針としては不整合ではないが、移行時の意味変更が大きい
- future multi-vehicle support に進みやすいか:
  - race-wide summary が欲しい場合の受け皿にはなる
  - ただし aggregate 定義を先に決める必要があり、first slice の外側の論点を呼び込みやすい
- 曖昧さを減らせるか:
  - 明確な aggregate 定義を別途固定できれば減る
  - 現時点の情報だけでは、何を aggregate と呼ぶかが未確認のため小さく決めにくい

### Option B. Snapshot 的 Field として暫定維持する

current semantics を将来も暫定維持し、`RaceState.completed_laps` を primary vehicle snapshot 由来の補助 field として残す案です。

評価:

- race-wide responsibility と vehicle-local responsibility の分離:
  - 分離には最も合いにくい
  - `RaceState` に vehicle-local progress 由来の値が残り続ける
- current first slice docs との整合:
  - current docs とは最も整合する
  - 現状維持なので追加の意味変更を持ち込まない
- future multi-vehicle support に進みやすいか:
  - short term では進めやすい
  - ただし曖昧な field を将来にも持ち越しやすく、後続 issue の整理コストを残す
- 曖昧さを減らせるか:
  - current semantics を明文化している限り、読み方の誤解はある程度抑えられる
  - それでも field 名と message 所属が race-wide aggregate を期待させやすい

### Option C. Vehicle-Local Progress を `VehicleRaceStatus` に寄せる

`RaceState.completed_laps` の current semantics は当面そのまま据え置きつつ、future handling の方向としては `RaceState` を race-wide state observation に寄せ、vehicle-local lap progress の主責務を `VehicleRaceStatus` にさらに明確に寄せる案です。

この案は、この issue の中で field removal や `.msg` redesign を実施する意味ではありません。責務整理としての推奨方向だけを決めます。

評価:

- race-wide responsibility と vehicle-local responsibility の分離:
  - 最も合う
  - `RaceState` は race-wide state、`VehicleRaceStatus` は vehicle-local progress、`LapEvent` は event-only という current boundary を強化できる
- current first slice docs との整合:
  - current docs と整合する
  - すでに `VehicleRaceStatus` を主情報源とし、`RaceState.completed_laps` を snapshot 的に読む前提と矛盾しない
- future multi-vehicle support に進みやすいか:
  - 進みやすい
  - aggregate 定義を急がず、first slice の責務分離を保ったまま後続 issue に進める
- 曖昧さを減らせるか:
  - race-wide progress を `RaceState.completed_laps` に背負わせないので、field の意味拡張による曖昧さを増やしにくい
  - ただし field 自体の最終扱いは未決定のまま残る

## Comparison Summary

| Option | Responsibility separation | First slice alignment | Multi-vehicle forward path | Ambiguity reduction |
| --- | --- | --- | --- | --- |
| A. race-wide aggregate に再定義 | 中 | 中 | 中 | 低から中 |
| B. snapshot 的 field として暫定維持 | 低 | 高 | 低から中 | 低 |
| C. `RaceState` の lap progress responsibility を薄くし、`VehicleRaceStatus` に寄せる | 高 | 高 | 高 | 中から高 |

補足:

- Option A は race-wide aggregate の定義未確定が blocker になりやすい
- Option B は current semantics を延命しやすいが、未整理論点も延命しやすい
- Option C は current docs との整合を保ったまま、future direction だけを最小に固定しやすい

## Recommendation

推奨は **Option C** とします。

理由:

- current first slice docs がすでに示している責務境界と最も整合する
- `RaceState.completed_laps` を race-wide aggregate に見せかけたまま拡張しないで済む
- future multi-vehicle support に向けて、vehicle-local progress の主情報源を `VehicleRaceStatus` に固定しやすい
- aggregate 定義、schema redesign、ranking 系論点をこの issue に持ち込まずに済む

この推奨は、`RaceState.completed_laps` の immediate removal や semantic break を意味しません。current semantics は current docs のまま維持しつつ、future implementation では `RaceState` 側に lap progress responsibility を増やさない方針を採ります。

## What This Recommendation Does Not Decide

次はこの文書では未決定のまま残します。

- `RaceState.completed_laps` を将来 remove するか
- compatibility field として残すか
- race-wide aggregate field を別途追加する必要があるか
- aggregate が必要なら何を aggregate と呼ぶか
- `.msg` change や consumer migration をいつ行うか

これらは別 issue で扱うべきです。この文書は first slice の責務方向だけを固定します。

## Acceptance Criteria For A Follow-Up Implementation Issue

後続 implementation issue では、少なくとも次を満たすことを acceptance criteria とします。

1. `RaceState` を race-wide state observation、`VehicleRaceStatus` を vehicle-local progress の主情報源として扱う前提が docs と実装方針で一貫している。
2. 新しい multi-vehicle 関連の progress 判断を追加する場合、vehicle-local lap progress は `VehicleRaceStatus` を基準にする。
3. `RaceState.completed_laps` を race-wide aggregate として扱う新規仕様や新規説明を持ち込まない。
4. `RaceState.completed_laps` の current semantics を壊す変更をする場合は、別 issue で明示的に扱う。
5. `.msg` change、runtime behavior change、API redesign、ranking / leaderboard 論点を同じ issue に混ぜない。

## Deferred Work

後続 issue に送る deferred work は次のとおりです。

- `RaceState.completed_laps` を compatibility field として残すかどうかの判断
- race-wide aggregate progress が本当に必要かの判断
- 必要な場合に、aggregate definition を別 issue で固定すること
- `.msg` change の要否判断
- consumer migration が必要なら、その移行境界の整理

## Relationship To Current Semantics Doc

`docs/design/race_state_completed_laps_current_semantics.md` は current behavior の読み方を固定する文書です。  
この文書は future handling の推奨方向を固定する文書です。

したがって、current semantics は次のまま維持されます。

- `RaceState.completed_laps` は race-wide aggregate ではない
- primary vehicle snapshot 由来である
- race-wide completion の判断には使わない
- vehicle-local finish の主判断には `VehicleRaceStatus.has_finished` を使う
