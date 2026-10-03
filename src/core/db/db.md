# ERD

```mermaid
erDiagram
    categories ||--o{ records : "実績時間の記録"
    categories ||--o{ todo : ""
    todo ||--o{ records : "TODOと時間記録の関連付け(あれば)"
    categories ||--o{ goal : ""
    todo ||--o{ goal : "(category_id と排他)"
    goal ||--o{ plan : "goalから生成された予定(任意)"
    categories ||--o{ plan : ""
    categories ||--o{ constraints : "target_category_id: 制約の監視対象"
    categories ||--o{ constraints : "event_category_id: 休憩イベントの記録先"
    constraints ||--o{ constraint_tiers : "段階的な休憩ルール"
    plan ||--o{ records : "予定と実績の関連付け(あれば)"

    categories {
        integer id PK
        integer parent_id FK "id から親参照"
        text name
        integer is_hidden
        integer is_folder "0:タスク，1:フォルダ"
    }

    records {
        integer id PK
        integer category_id FK
        integer todo_id FK "どのTODOか(任意)"
        integer plan_id FK "どのplanの実行か(任意)"
        text time_begin
        text time_end
        text memo
    }

    todo {
        integer id PK
        integer category_id FK
        integer status "0:未完了，1:完了"
        integer priority
        text todo_name
        text start_time
        text target_end
        text deadline
        text completion_date
        text memo
    }

    goal {
        integer id PK
        integer category_id FK "category_id / todo_id はどちらか一方のみ設定。フォルダを選択したらそれ以下のcategoryの実績も合算して進捗計算(子に個別goalがあっても二重カウントでよい)"
        integer todo_id FK
        text goal_name "任意(空文字可)。同じ対象に複数のgoalを持てる"
        integer period_type "0:DAILY, 1:WEEKLY, 2:MONTHLY, 3:EVERY_N_DAYS"
        integer period_n "period_type=3のとき、3日に1回など"
        integer target_time "期間ごとの目標(単位:sec)"
        text start_time "開始日時(UTC)"
        text end_time "終了日時(UTC)。NULL=期限なし"
        integer is_active "有効1/無効0"
        text memo "メモ"
    }

    plan {
        integer id PK
        integer goal_id FK "NULL可。目標に紐付かない単独の予定も許容"
        integer category_id FK "goal経由でなくても必須で持つ"
        text plan_name
        text start_time
        text target_end
        text memo
    }

    constraints {
        integer id PK
        integer target_category_id FK "制約(閾値判定)の対象カテゴリ。NULL可=全カテゴリ。フォルダを選択したらそれ以下のcategoryに適用"
        integer event_category_id FK "閾値超過時に発生させるイベント(強制休憩など)を記録するカテゴリ"
        integer constraint_type "0:連続稼働→段階的休憩ルール, 1:期間累計上限"
        integer period_type "type=1用。0:DAILY,1:WEEKLY,2:MONTHLY"
        integer period_cap_time "type=1用。期間内合計の上限(例:月60h→3600)"
        text memo
    }

    constraint_tiers {
        integer id PK
        integer constraint_id FK
        integer threshold_time "累積稼働がこの値以上になったら適用(降順で最初に一致した行を採用)"
        integer required_time "挿入すべき休憩時間"
    }
```
