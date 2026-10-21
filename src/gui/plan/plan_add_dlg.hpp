#pragma once
#include <wx/wx.h>
#include <wx/datectrl.h>
#include <wx/listctrl.h>
#include <functional>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include "core/db/database.hpp"
#include "core/plan/plan_input.hpp"

// Plan の Add Row と、行の編集 (F2 / ダブルクリック) で共通に使う入力画面。
// 年・月・日・時・分・秒は、すべて別々の入力欄 (それぞれにラベル付き)。桁がそろうと、次の欄へ進む。
//
// 【追加モード】日付の組と時間帯の組を掛け合わせて、まとめて予定を作る。
//   - 日付の入力方法 (wxChoice): 1 日だけ / 複数の日 (好きな日を並べる) / 繰り返し (起点・回数・間隔)
//   - 時間帯の入力方法 (wxChoice): 1 つ / 複数 (好きなだけ並べる)。終了の時は 24 以降も書ける (26 = 翌日の 2 時)
//   - 選んだ入力方法に合わせて、入力欄の並びが自動で変わる
//   - すべての日付 x すべての時間帯の予定が、下の一覧(プレビュー)に出る。重なる予定は赤
//   - F2 = 追加して閉じる / F3 = 追加して開いたまま(続けて入力) / F12 = 閉じる
// 【編集モード】1 つの行の、カテゴリ・日付・時間帯・メモを直す。日付と時間帯は 1 つずつ (入力方法の選択は出ない)。
//   - F2 = 反映して閉じる / F12 = 閉じる
// 【複数行の編集モード】選んだ複数の行を、まとめて直す。入力欄には、先頭の行の値が入る (行ごとに値が違う欄は黄色)。
//   自分で触った欄だけが、全行に反映される (日付 = その日に移す / Start Time・End Time = その時刻にする / Category・Memo = 置き換える)。
//   触らなかった欄は、各行の値のまま。
//   下の一覧(プレビュー)の行をクリックすると、その行の日時が入力欄に入り、その行だけを直せる (「All rows」で全行に戻る)。
//   行ごとの修正は、全行向けの修正より優先する
class PlanAddDlg : public wxDialog {
public:
    struct Entry {
        wxDateTime start;   // ローカル時刻
        wxDateTime end;
    };
    struct Request {
        int category_id = -1;
        wxString memo;
        std::vector<Entry> entries;   // 編集モードでは、必ず 1 件。複数行の編集モードでは、選んだ行と同じ数・同じ順
        // 複数行の編集モードのとき: category_id が -1 なら、各行のカテゴリを変えない。apply_memo が false なら、各行のメモを変えない
        bool apply_memo = true;
    };
    // 編集モードの、直す前の内容
    struct EditInitial {
        // 複数行の編集モードのときの、1 行ぶん
        struct Original {
            int row_number = 0;       // 表の行番号 (1 始まり)
            int category_id = -1;
            wxDateTime start;
            wxDateTime end;
            wxString memo;
        };

        int row_number = 0;           // 表の行番号 (1 始まり。題名に出す)
        int category_id = -1;
        wxDateTime start;
        wxDateTime end;
        wxString memo;
        std::vector<Original> originals;   // 2 件以上入っていると、複数行の編集モードになる (上の 5 つは使わない)
    };

    // on_add: 追加(編集モードでは反映)を頼まれたときに呼ぶ。呼び出し側が、表に反映する
    // existing_spans: 指定したカテゴリの、すでにある予定の [開始, 終了) (epoch 秒)。重なりの目印に使う
    using AddCallback = std::function<void(const Request&)>;
    using SpansCallback = std::function<std::vector<std::pair<long long, long long>>(int category_id)>;

    // 追加モード
    PlanAddDlg(wxWindow* parent, Database& db, int default_category_id, AddCallback on_add, SpansCallback existing_spans);
    // 編集モード
    PlanAddDlg(wxWindow* parent, Database& db, const EditInitial& initial, AddCallback on_apply, SpansCallback existing_spans);

private:
    // 2 つの公開コンストラクタの実体
    PlanAddDlg(wxWindow* parent, Database& db, bool edit, const EditInitial& initial,
               AddCallback on_add, SpansCallback existing_spans);

    // 日付 1 つぶんの入力欄 (yyyy / mmdd)
    struct DateRow {
        wxBoxSizer* sizer = nullptr;
        wxTextCtrl* year = nullptr;        // yyyy
        wxTextCtrl* monthday = nullptr;    // mmdd (ちょうど 4 桁)
        wxButton* remove = nullptr;        // 複数の日の行だけ
    };
    // 時間帯 1 つぶんの入力欄 (開始の hhmm / ss、終了の hhmm / ss)
    struct TimeRow {
        wxBoxSizer* sizer = nullptr;
        wxTextCtrl* start_hhmm = nullptr;  // hhmm (930 や 9 のような短い入力も可)
        wxTextCtrl* start_ss = nullptr;    // ss (空なら 0)
        wxTextCtrl* end_hhmm = nullptr;    // 終了の時は 24 以降も可 (2600 = 翌日の 2 時)
        wxTextCtrl* end_ss = nullptr;
        wxButton* remove = nullptr;        // 複数の時間帯の行だけ
    };
    // 複数行の編集で、入力欄が持つ 6 つの項目
    enum Component { COMP_YEAR = 0, COMP_MONTHDAY, COMP_START_HHMM, COMP_START_SS, COMP_END_HHMM, COMP_END_SS, COMP_COUNT };
    // 1 つの対象 (全行 / ある 1 行) の入力欄の内容。touched は、触った項目の印 (COMP_xxx の番号のビット)
    struct FieldSet {
        wxString text[COMP_COUNT];
        unsigned touched = 0;
    };
    // FieldSet を読み取った結果。has[i] が true の項目だけ、値を持つ
    struct Applied {
        bool has[COMP_COUNT] = {false, false, false, false, false, false};
        int year = 0;
        int month = 0;
        int day = 0;
        int start_hhmm = 0;   // 0 時からの秒 (時分の部分)
        int start_ss = 0;
        int end_hhmm = 0;
        int end_ss = 0;
    };
    enum DateMode { DATE_ONE = 0, DATE_SEVERAL, DATE_REPEAT };
    enum TimeMode { TIME_ONE = 0, TIME_SEVERAL };

    Database& m_db;
    AddCallback m_on_add;
    SpansCallback m_existing_spans;
    const bool m_edit;                 // true: 編集モード (複数行を含む)
    const EditInitial m_initial;       // 編集モードの、直す前の内容
    const bool m_multi;                // true: 複数行の編集モード
    // 複数行の編集で、いま入力欄が編集している対象の、利用者が触った入力欄 (打ち込んだ欄)。触った欄だけを反映する
    std::set<const wxTextCtrl*> m_touched;
    bool m_category_touched = false;   // 複数行の編集で、カテゴリを選び直した
    FieldSet m_batch;                  // 複数行の編集: 全行に反映する内容 (入力欄が全行を編集していないときの退避先)
    std::vector<FieldSet> m_row_sets;  // 複数行の編集: 行ごとに、その行だけに反映する内容
    int m_active_row = -1;             // 入力欄がいま編集している対象。-1 = 全行 / 0 以上 = 下の一覧のその行だけ
    bool m_same[COMP_COUNT] = {true, true, true, true, true, true}; // 全行で同じ値だった項目 (違う項目は黄色)
    bool m_memo_same = true;
    bool m_loading_list = false;       // 一覧の選択を、プログラムが変えている最中 (選択のイベントを無視する)
    wxStaticText* m_st_target = nullptr;   // 「Editing: all N rows」/「Editing: row N only」
    wxButton* m_btn_all_rows = nullptr;
    bool m_ready = false;              // 画面を作り終えた (作っている途中は、プレビューを更新しない)

    int m_category_id;
    wxStaticText* m_st_category;
    wxTextCtrl* m_memo;

    // 日付
    wxChoice* m_date_mode;
    int m_date_mode_current = DATE_ONE;
    wxBoxSizer* m_date_one_sizer;      // 1 日だけ
    DateRow m_one_day;
    wxBoxSizer* m_date_several_sizer;  // 複数の日
    wxBoxSizer* m_date_rows_sizer;
    std::vector<DateRow> m_date_rows;
    wxBoxSizer* m_date_repeat_sizer;   // 繰り返し
    DateRow m_repeat_start;
    wxTextCtrl* m_repeat_count;
    wxTextCtrl* m_repeat_step;
    wxChoice* m_repeat_unit;           // days / weeks / months
    wxDatePickerCtrl* m_pick_date;     // カレンダーで日付を選ぶ。入力方法によって、入力欄を埋める / 行を足す / 起点にする
    wxButton* m_btn_pick;

    // 時間帯
    wxChoice* m_time_mode;
    int m_time_mode_current = TIME_ONE;
    wxBoxSizer* m_time_one_sizer;      // 1 つ
    TimeRow m_one_time;
    wxBoxSizer* m_time_several_sizer;  // 複数
    wxBoxSizer* m_time_rows_sizer;
    std::vector<TimeRow> m_time_rows;

    // プレビュー
    wxListCtrl* m_list;
    wxStaticText* m_st_error;
    wxStaticText* m_st_summary;
    wxStaticText* m_st_status;
    // カテゴリごとの、すでにある予定の [開始, 終了)。カテゴリが変わるまで取り直さない
    std::map<int, std::vector<std::pair<long long, long long>>> m_spans_by_category;
    const std::vector<std::pair<long long, long long>>& SpansOf(int category_id);

    // 入力欄の作成
    // label が空なら、ラベルは付けない。tooltip は、マウスを重ねたときの書式の説明
    wxTextCtrl* CreateField(wxBoxSizer* row, const wxString& label, int digits, const wxString& initial,
                            const wxString& tooltip = wxString());
    void LinkFields(const std::vector<wxTextCtrl*>& chain, const std::vector<int>& digits); // 桁がそろったら、次の欄へ
    DateRow CreateDateRow(const wxDateTime& initial, bool removable);
    TimeRow CreateTimeRow(int start_seconds, int end_seconds, bool removable); // 0 時からの秒 (終了は 86400 以上もある)
    void SetDateRow(const DateRow& row, const wxDateTime& dt);
    void SetTimeRow(const TimeRow& row, int start_seconds, int end_seconds);

    // 入力欄 -> 値。不正なら false と、理由 (where は「Day 2」のような場所の名前)
    bool ReadDate(const DateRow& row, PlanInput::Date& out, const wxString& where, wxString& error) const;
    bool ReadTime(const TimeRow& row, PlanInput::TimeRange& out, const wxString& where, wxString& error) const;
    bool ReadPrimaryDate(wxDateTime& out) const;          // いまの入力方法の、先頭の日付 (入力方法を切り替えるとき引き継ぐ)
    bool ReadPrimaryTime(PlanInput::TimeRange& out) const;

    // 入力方法の切り替え。選んだ方法の入力欄だけを出し、レイアウトを組み直す
    void ApplyDateMode(int new_mode);
    void ApplyTimeMode(int new_mode);
    void AddDateRow(const wxDateTime& initial);
    void AddTimeRow(int start_seconds, int end_seconds);
    void RemoveDateRow(wxButton* button);
    void RemoveTimeRow(wxButton* button);
    void PickDate();                                       // カレンダーの日付を、入力方法に合わせて反映する

    // 入力から、追加する予定の一覧を作る。問題があれば error に理由を入れる
    std::vector<Entry> BuildEntries(wxString& error) const;
    // 複数行の編集モード: 選んだ行ごとの、直したあとの日時。空欄の項目は、その行の値のまま
    std::vector<Entry> BuildMultiEntries(wxString& error) const;
    // 全行向けの内容 (batch) と、行ごとの内容 (rows) から、直したあとの日時を作る。行ごとの内容が、全行向けより優先する
    std::vector<Entry> BuildMultiEntriesFrom(const FieldSet& batch, const std::vector<FieldSet>& rows, wxString& error) const;
    bool ParseSet(const FieldSet& set, Applied& out, const wxString& where, wxString& error) const;
    wxTextCtrl* ComponentField(int component) const;
    void StoreActive();                    // 入力欄の内容を、いまの対象 (全行 / 選んだ行) に退避する
    void LoadTarget();                     // いまの対象の内容を、入力欄に出す
    void SelectTarget(int row);            // 入力欄の編集対象を切り替える (-1 = 全行)
    void RefreshMixedColours();            // 値が行ごとに違う項目を、黄色にする (全行を編集しているときだけ)
    void UpdateTargetLabel();
    int CategoryOfEntry(size_t index) const;   // 予定 index のカテゴリ (複数行で変えないときは、その行のカテゴリ)
    void UpdatePreview();
    void ChooseCategory();
    void SetStatus(const wxString& text);
    void DoAdd(bool close);
};
