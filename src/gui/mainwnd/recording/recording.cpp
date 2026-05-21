#include "recording.hpp"
#include "core/db/database.hpp"
#include "gui/mainwnd/mainwnd.hpp"
#include "gui/mainwnd/inspector/inspector.hpp"
#include <wx/dataview.h>
#include <wx/event.h>
#include <wx/msgdlg.h>
#include <wx/timer.h>
#include <wx/variant.h>
#include <wx/wx.h>

Recording::Recording(wxWindow* parent, Database &dbRef) : wxPanel(parent) , m_db(dbRef) {
	Bind(wxEVT_TIMER, &Recording::OnTimer, this);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* dvlc_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* ctrl_sizer = new wxBoxSizer(wxHORIZONTAL);

	m_dvlc = new wxDataViewListCtrl(this, wxID_ANY);

	// 列の定義
	m_dvlc->AppendTextColumn(_("Record ID"), wxDATAVIEW_CELL_INERT, FromDIP(75));
	m_dvlc->AppendTextColumn(_("ID"), wxDATAVIEW_CELL_INERT, FromDIP(75));
	m_dvlc->AppendTextColumn(_("Category Name"), wxDATAVIEW_CELL_INERT, FromDIP(150));
	m_dvlc->AppendTextColumn(_("Start Time"), wxDATAVIEW_CELL_INERT, FromDIP(175));
	m_dvlc->AppendTextColumn(_("Elapsed Time"), wxDATAVIEW_CELL_INERT, FromDIP(150));

	m_dvlc->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &Recording::OnSelectionChanged, this); // 行選択イベント

	dvlc_sizer->Add(m_dvlc, 1, wxEXPAND);
	sizer->Add(dvlc_sizer, 1, wxEXPAND);

	wxStaticText* record_id = new wxStaticText(this, wxID_ANY, "Record ID:");
	m_st_rid = new wxStaticText(this, wxID_ANY, "-");
	m_btn_start = new wxButton(this, wxID_ANY, wxT("▶"));
	m_btn_stop = new wxButton(this, wxID_ANY, wxT("■"));

	m_btn_start->Bind(wxEVT_BUTTON, &Recording::OnStartRecordFromBtn, this);
	m_btn_stop->Bind(wxEVT_BUTTON, &Recording::OnStopRecord, this);

	ctrl_sizer->AddStretchSpacer();
	ctrl_sizer->Add(record_id, 0, wxALIGN_CENTER_VERTICAL);
	ctrl_sizer->Add(m_st_rid, 0, wxALIGN_CENTER_VERTICAL);
	ctrl_sizer->AddSpacer(FromDIP(10));
	ctrl_sizer->Add(m_btn_start, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));
	ctrl_sizer->Add(m_btn_stop, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));

	sizer->Add(ctrl_sizer, 0, wxEXPAND | wxALL, FromDIP(10));
	SetSizer(sizer);
}

// Recording 側の start ボタンから Record を開始する場合。
// Inspector から、ID と名前を取得し、OnStartへ。->Record 開始
void Recording::OnStartRecordFromBtn(wxCommandEvent& event){
	wxCommandEvent evt(wxEVT_MENU, ID_START_RECORDING);
	// id と name は Mainwnd が Inspector から取って Recording に渡す
	wxPostEvent(GetParent(), evt);
}

// 外部から Record を開始する場合。
void Recording::OnStartRecord(int id, const wxString& name) {
	m_selected_id = id; // ほかに引き渡す用途
	int is_folder = this->m_db.IsFolder(m_selected_id);

	if (is_folder == 1) {
	    wxMessageBox(
			_("Please select a task to start recording. Folders cannot be tracked."),
			"Record Error",
			wxOK | wxICON_ERROR,
			this
		    );
	    return;
	}

	long long recordId = this->m_db.StartRecord(m_selected_id);

	if (recordId == -1) {
		wxMessageBox(
				_("Failed to start record"),
				"Record Error",
				wxOK | wxICON_ERROR,
				this
			    );
		return;
	}

	wxDateTime now = wxDateTime::Now();
	wxString startTimeStr = now.Format("%Y-%m-%d %H:%M:%S");
	
	wxVector<wxVariant> data;
	data.push_back(wxString::Format("%lld", recordId));
	data.push_back(wxString::Format("%d", id));        // ID (Category ID)
	data.push_back(name);                               // Category Name
	data.push_back(startTimeStr);                       // Start Time
	data.push_back("00:00:00");                         // Elapsed Time (初期値)

	m_dvlc->AppendItem(data);

	// 画面下部の現在の Record ID 表示も更新
	m_st_rid->SetLabel(wxString::Format("%lld", recordId));

	// 1s 周期でタイマ
	m_timer.Start(1000);
	// this->Bind(wxEVT_TIMER, &Recording::OnTimer, this, m_timer.GetId());
}

void Recording::OnSelectionChanged(wxDataViewEvent& event) {
	int row = m_dvlc->GetSelectedRow();

	if (row == wxNOT_FOUND) {
		m_st_rid->SetLabel("0");
		return;
	}

	wxVariant varId;
	m_dvlc->GetValue(varId, row, 0); // rec ID 取得
	m_st_rid->SetLabel(varId.GetString());
}

void Recording::OnStopRecord(wxCommandEvent& event) {
	// リストで現在選択されている行を取得
	int row = m_dvlc->GetSelectedRow();

	if (row == wxNOT_FOUND) {
		wxMessageBox(_("Please select a running record to stop."), _("Info"), wxOK | wxICON_INFORMATION);
	return;
	}

	wxVariant varId;
	m_dvlc->GetValue(varId, row, 0);
	long long recordId = 0;
	if (!varId.GetString().ToLongLong(&recordId)) {
		// 万が一、数値に変換できない文字列が入っていた場合のガード
		return; 
	}

	if(m_db.EndRecord(recordId)) {
		m_dvlc->DeleteItem(row);

		// 削除後は選択が消えるので、表示リセット
		m_st_rid->SetLabel("-");

		if (m_dvlc->GetItemCount() == 0) {
			m_timer.Stop();
		}
	}
	// 統計情報自動更新
	// CheckBox のチェック有無は、dashboard 側で管理してる
	wxCommandEvent evt(wxEVT_MENU, ID_UPDATE_STATISTIC);
	evt.SetEventObject(this);
	wxPostEvent(GetParent(), evt);
}

void Recording::OnTimer(wxTimerEvent& event) {
	int rowCount = m_dvlc->GetItemCount();
	if (rowCount == 0) return;

	wxDateTime now = wxDateTime::Now();

	for (int i = 0; i < rowCount; ++i) {
		wxVariant startVar;
		m_dvlc->GetValue(startVar, i, 3);

        
	wxDateTime startTime;
        if (startTime.ParseFormat(startVar.GetString(), "%Y-%m-%d %H:%M:%S")) {
		wxTimeSpan diff = now.Subtract(startTime);
		m_dvlc->SetValue(diff.Format("%H:%M:%S"), i, 4);
        }
    }
    m_dvlc->Refresh();
}
