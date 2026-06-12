// database の接続テストです．CLI 専用です．wxWidgets 側では使いません．
#include <cstdio>
#include <ctime>
#include <ios>
#include <iostream>
#include <string>
#include <limits>
#include "core/db/database.hpp"

static std::string now_utc_iso8601() {
    time_t t = time(nullptr);
    struct tm* tm_utc = gmtime(&t);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_utc);
    return std::string(buf);
}

void init(const std::string& path);
void connect(const std::string& path);
void inscat(const std::string& path);
void insrec(const std::string& path);
void test_record_list(const std::string& path);

int main(){
	std::string path;
	std::cout << "DB テスト" << std::endl;
	std::cout << "DB のパスを入力してください。" << std::endl;
	std::cin >> path;
	
	std::cout << "実行するテストを選択してください" << std::endl;

	int testcase;
	std::cout << "1." << "DB初期化テスト" << std::endl;
	std::cout << "2." << "DB接続テスト" << std::endl;
	std::cout << "3." << "カテゴリ登録テスト" << std::endl;
	std::cout << "4." << "レコードテスト" << std::endl;
	std::cout << "5." << "GetRecordList テスト" << std::endl;
	std::cin >> testcase;

	switch (testcase) {
		case 1:
			std::cout << "DB 初期化テストを開始します。" << std::endl;
			init(path);
			break;
		case 2:
			std::cout << "DB 接続テストを開始します。" << std::endl;
			connect(path);
			break;
		case 3:
			std::cout << "カテゴリ登録テストを開始します．" << std::endl;
			inscat(path);
			break;
		case 4:
			std::cout << "レコードテストを開始します．" << std::endl;
			insrec(path);
			break;
		case 5:
			std::cout << "GetRecordList テストを開始します。" << std::endl;
			test_record_list(path);
			break;
	}
	return 0;
}

void init(const std::string& path){

	Database db; // インスタンス作成
	
	if(db.Connect(path)){
		std::cout << "オープン成功" << std::endl;
	} else {
		std::cout << "オープン失敗" << std::endl;
		std::cout << "新規作成します" << std::endl;

		if (db.Create(path)){ // なかったら作る
			std::cout << "作成成功" << std::endl;
			std::cout << "ファイル名: " << path << std::endl;
		} else {
			std::cerr << "DB作成失敗" << std::endl;
		}

		// テーブル初期化のテスト
		if (db.Initialize()) {
			std::cout << "テーブルの初期化に成功しました。" << std::endl;
		}
	
		db.Close(); // 閉じる
	}

}

void connect(const std::string& path){
	Database db; // インスタンス作成
	
	if (db.Connect(path)) {
		std::cout << "DB接続成功" << std::endl;
	} else {
		std::cout << "DB接続失敗" << std::endl;
	}
	db.Close(); // 閉じる
}


void inscat(const std::string& path){
	Database db;
	if (!db.Connect(path)){ // 接続
		std::cout << "DB接続失敗" << std::endl;
		return;
	}

	std::string name;
	int parent_id;

	std::cout << "カテゴリ名称: ";
	std::cin >> name;

	std::cout << "親のID(なければ0): ";
	std::cin >> parent_id;

	if (db.InsertCategories(parent_id, name, false)){
		std::cout << "登録成功" << std::endl;
	} else {
		std::cout << "登録失敗" << std::endl;
	}

	db.Close();

}


void test_record_list(const std::string& path) {
	Database db;
	if (!db.Connect(path)) {
		std::cout << "DB接続失敗" << std::endl;
		return;
	}

	std::string start = "2020-01-01 00:00:00";
	std::string end   = "2030-01-01 00:00:00";

	auto records = db.GetRecordList(start, end);
	std::cout << "取得件数: " << records.size() << std::endl;
	for (const auto& r : records) {
		std::cout << r.id << " | " << r.category_name
		          << " | " << r.time_begin << " (epoch)"
		          << " | " << r.time_end << " (epoch)"
		          << " | " << r.total_seconds << "s" << std::endl;
	}
	db.Close();
}

void insrec(const std::string& path){
	Database db;
	if (!db.Connect(path)){ // 接続
		std::cout << "DB接続失敗" << std::endl;
		return;
	}

	int category_id;
	std::string time_begin;
	std::string time_end;

	std::cout << "カテゴリID: ";
	std::cin >> category_id;
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // category_id の改行破棄 (std::cin.get()と干渉するため)

	std::cout << "Enter で開始: ";
	std::cin.get();
	time_begin = now_utc_iso8601();
	

	std::cout << "Enter で終了: ";
	std::cin.get();
	time_end = now_utc_iso8601();

	if (time_end < time_begin) {
		std::cerr << "時刻逆転エラー" << std::endl;
		return;
	}

	if (db.InsertRecords(category_id, time_begin, time_end)){
		std::cout << "登録成功" << std::endl;
	} else {
		std::cout << "登録失敗" << std::endl;
	}

	db.Close();
}
