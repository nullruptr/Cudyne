# Cudyne

**Cu**be + **Dyne** (unit of force, 1 dyn = 10⁻⁵ N)

![C++](https://img.shields.io/badge/C++-00599C?style=flat&logo=c%2B%2B&logoColor=white)
![Windows](https://img.shields.io/badge/Windows_11-0078D4?style=flat&logo=windows&logoColor=white)
![License](https://img.shields.io/badge/license-MIT-green)

> A dynamic task & time manager.

本ソフトウェアは、時間を管理・記録し、フィードバックするためのツールです。  
いつ何をしたのかを記録することで、何に時間を使いすぎているのか、あるいは何ができていないのかを数値データとして把握できます。

> [!WARNING]  
> 現在開発途中です。

## 動作要件

- Windows 11 24H2 以降

## クレジット

- [wxWidgets](https://github.com/wxWidgets/wxWidgets/releases/tag/v3.2.8)
- [SQLite](https://sqlite.org/)
- [SOCI](https://github.com/SOCI/soci)

### ビルド

[myIDE](https://github.com/nullruptr/myIDE)にて、エディタを提供しています。また、Docker を用いることで、簡単にコンパイルすることができます。

## Windows におけるビルドの再現手順 (MSVC + Conan)

MinGW クロスビルド（`docker/mingw/Dockerfile` 参照）とは別に、Windows ネイティブで
MSVC + [Conan](https://conan.io/) を用いてビルドすることもできます。
依存ライブラリ（wxWidgets 3.2.10 / SQLite3 3.53.3 / SOCI 4.1.2）は Conan が
ConanCenter からソース取得・ビルドします。

### 必要環境

- Windows 10 以降
- [Visual Studio 2022](https://visualstudio.microsoft.com/) (「C++によるデスクトップ開発」ワークロード)
  - Ninja と rc.exe は VS に同梱されているため別途インストール不要
- [Python](https://www.python.org/) 3.13 以降
- [Poetry](https://python-poetry.org/)
- CMake 3.20 以降 ([https://cmake.org/download/](https://cmake.org/download/) または `winget install -e --id Kitware.CMake`)(参考: https://winget.run/pkg/Kitware/CMake)
- Git (`winget install --id Git.Git -e --source winget`) (参考: https://git-scm.com/install/windows)

以降の手順はすべて **"Developer PowerShell for VS 2022"** から実行してください。
（`cl.exe` / `rc.exe` / `ninja.exe` を PATH に通すためです）

### 1. Poetry のインストール

以下を実行します。(参考: https://python-poetry.org/docs/#installing-with-the-official-installer)

```powershell
(Invoke-WebRequest -Uri https://install.python-poetry.org -UseBasicParsing).Content | py -
```

インストール後、`poetry --version` が表示されることを確認してください。

### 2. 依存関係のインストール (Conan を含む)

このリポジトリの `pyproject.toml` には Conan が依存関係として登録済みなので、
`poetry install` で Conan がインストールされます。

```powershell
poetry install
```

```powershell
poetry run conan --version
```

### 3. Conan プロファイルの確認

`profiles/` 以下に 3 つの Conan プロファイル（いずれも MSVC 19.3x / x64）を用意しています。

- `profiles/msvc-release` : ホスト (実行ファイル) 用 Release プロファイル（ランタイム static）
- `profiles/msvc-debug` : ホスト (実行ファイル) 用 Debug プロファイル（ランタイム static）
- `profiles/msvc-build` : ビルドツール (gettext 等の tool_requires) 用プロファイル

`-pr:b` を指定しない場合、ビルドツール類は `~/.conan2/profiles/default` を使ってビルドされ、
日本語環境 (コードページ 932) 特有の文字化けビルドエラーを避けるための `/utf-8` 指定が
適用されません。そのため常に `-pr:h`（ホスト）と `-pr:b`（ビルド）の両方を指定してください。

環境の Visual Studio バージョンが異なる場合は、3つのプロファイルすべての
`compiler.version` を適宜変更してください。

### 4. Release 版のビルド

```powershell
make -f Makefile.msvc release
```

生成された実行ファイルは `build/release/build/Release/Cudyne.exe` に出力されます。

初回は wxWidgets 等を ConanCenter の binary が無い設定のためソースからビルドするので、
時間がかかる点に注意してください（数十分程度）。2回目以降は Conan のキャッシュ
（`~/.conan2/p`）が再利用されるため高速になります。

### 5. Debug 版のビルド

```powershell
make -f Makefile.msvc debug
```

実行ファイルは `build/debug/build/Debug/Cudyne.exe` に出力されます。

### 6. Neovim 用 compile_commands.json の生成 (Ninja)

Visual Studio ジェネレータは `CMAKE_EXPORT_COMPILE_COMMANDS` に対応していないため、
clangd (Neovim) 用の `compile_commands.json` は Ninja ジェネレータを使って
別途生成します。

```powershell
make -f Makefile.msvc compdb
```

生成された `compile_commands.json` はリポジトリ直下にコピーされます
(`.gitignore` 済みなのでコミットされません)。

### Makefile.msvc について

| ターゲット | 内容                                                                    |
| ---------- | ----------------------------------------------------------------------- |
| `release`  | Release 版を Visual Studio ジェネレータでビルド                         |
| `debug`    | Debug 版を Visual Studio ジェネレータでビルド                           |
| `compdb`   | Ninja ジェネレータで `compile_commands.json` を生成（clangd/Neovim 用） |
| `clean`    | `build/` ディレクトリを削除                                             |

`CMakeLists.txt` 側では `-DMSVC_BUILD=ON` を渡すことで、MinGW クロスビルド用の設定
（`docker/mingw/Dockerfile`）を無効化し、Conan (`CMakeDeps`/`CMakeToolchain`) が
生成したターゲット (`wxWidgets::wxWidgets` / `SOCI::soci_core_static` /
`SOCI::soci_sqlite3_static` / `SQLite::SQLite3`) を利用するようになっています。
