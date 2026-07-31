# ERD

```mermaid
erDiagram
    categories ||--o{ records : "実績時間の記録"
    categories ||--o{ todo : ""
    todo ||--o{ records : "TODOと時間記録の関連付け(あれば)"

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
```
