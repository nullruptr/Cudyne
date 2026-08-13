# Cudyne

**Cu**be + **Dyne** (unit of force, 1 dyn = 10⁻⁵ N)

![C++](https://img.shields.io/badge/C++-00599C?style=flat&logo=c%2B%2B&logoColor=white)
![Windows](https://img.shields.io/badge/Windows_11-0078D4?style=flat&logo=windows&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-FCC624?style=flat&logo=linux&logoColor=black)
![License](https://img.shields.io/badge/license-MIT-green)

> A dynamic task & time manager.

本ソフトウェアは、時間を管理・記録し、フィードバックするためのツールです。  
いつ何をしたのかを記録することで、何に時間を使いすぎているのか、あるいは何ができていないのかを数値データとして把握できます。

> [!WARNING]  
> 現在開発途中です。

## 動作要件

- Windows 11 24H2 以降
- Linux (Ubuntu 24.04 以降で動作確認)

## クレジット

- [wxWidgets](https://github.com/wxWidgets/wxWidgets/releases/tag/v3.2.8)
- [SQLite](https://sqlite.org/)
- [SOCI](https://github.com/SOCI/soci)

### ビルド

[myIDE](https://github.com/nullruptr/myIDE)にて、エディタを提供しています。

Windows / Linux いずれも [Poetry](https://python-poetry.org/) + [Conan](https://conan.io/) で
依存ライブラリ（wxWidgets 3.2.10 / SQLite3 3.53.3 / SOCI 4.1.2）を ConanCenter から
取得・ビルドする構成です。

## Windows におけるビルドの再現手順 (MSVC + Conan)

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

### 6. compile_commands.json の生成 (Ninja)

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

`CMakeLists.txt` 側では `-DMSVC_BUILD=ON` を渡すことで、Conan
(`CMakeDeps`/`CMakeToolchain`) が生成したターゲット (`wxWidgets::wxWidgets` /
`SOCI::soci_core_static` / `SOCI::soci_sqlite3_static` / `SQLite::SQLite3`) を
利用するようになっています。

## Linux (Ubuntu) におけるビルド手順 (GCC + Conan)

Windows と同様に、Linux でも Poetry + Conan を用いてネイティブビルドします。

### 必要環境

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build python3 python3-venv pkg-config git \
    libgtk-3-dev \
    libx11-dev libx11-xcb-dev libfontenc-dev libice-dev libsm-dev libxau-dev libxaw7-dev \
    libxkbfile-dev libxmu-dev libxmuu-dev libxpm-dev libxss-dev libxt-dev libxv-dev \
    libxxf86vm-dev libxcb-glx0-dev libxcb-render-util0-dev libxcb-xkb-dev libxcb-icccm4-dev \
    libxcb-image0-dev libxcb-keysyms1-dev libxcb-randr0-dev libxcb-shape0-dev libxcb-sync-dev \
    libxcb-xfixes0-dev libxcb-xinerama0-dev libxcb-dri3-dev libxcb-cursor-dev libxcb-dri2-0-dev \
    libxcb-present-dev libxcb-composite0-dev libxcb-ewmh-dev libxcb-res0-dev \
    libxcb-util-dev libxcb-util0-dev
```

- Python 3.13 以降 (Poetry が要求)
- [Poetry](https://python-poetry.org/)
- CMake 3.20 以降、Ninja
- `libgtk-3-dev` ほか : wxWidgets (GTK バックエンド) や Conan の `xorg/system` パッケージが
  ビルド時に要求する X11/GTK 関連の開発パッケージ。ここに列挙したもの以外で不足しているパッケージが
  あれば、`conan install` 実行時のエラーメッセージに `apt install` すべきパッケージ名が表示されます。

### 1. Poetry のインストール

(参考: https://python-poetry.org/docs/#installing-with-the-official-installer)

```bash
curl -sSL https://install.python-poetry.org | python3 -
```

インストール後、`poetry --version` が表示されることを確認してください。

> [!NOTE]
> `poetry: そのようなファイルやディレクトリはありません` と表示される場合、
> インストール先の `~/.local/bin` に PATH が通っていません。新しいターミナルを
> 開き直すか、以下を実行してから再度試してください。
>
> ```bash
> source ~/.bashrc
> poetry --version
> ```
>
> それでも解決しない場合は、`~/.bashrc` に以下を追記してください。
>
> ```bash
> if [ -d "$HOME/.local/bin" ] ; then
>     PATH="$HOME/.local/bin:$PATH"
> fi
> ```

### 2. 依存関係のインストール (Conan を含む)

```bash
poetry install
```

```bash
poetry run conan --version
```

### 3. Conan デフォルトプロファイルの検出

Linux 側は Windows (MSVC) と異なりコンパイラのバージョンを固定する必要がないため、
`profiles/linux-*` は環境の Conan デフォルトプロファイル (`~/.conan2/profiles/default`)
を `include(default)` で取り込む構成にしています。まだ検出したことがなければ、
初回のみ以下を実行してください。

```bash
poetry run conan profile detect
```

`profiles/` 以下には以下の 3 つの Conan プロファイルを用意しています。

- `profiles/linux-release` : ホスト (実行ファイル) 用 Release プロファイル
- `profiles/linux-debug` : ホスト (実行ファイル) 用 Debug プロファイル
- `profiles/linux-build` : ビルドツール (tool_requires) 用プロファイル

### 4. Release 版のビルド

```bash
make release
```

生成された実行ファイルは `build/release/build/Release/Cudyne` に出力されます。

初回は wxWidgets 等を ConanCenter の binary が無い設定のためソースからビルドするので、
時間がかかる点に注意してください（数十分程度）。2回目以降は Conan のキャッシュ
（`~/.conan2/p`）が再利用されるため高速になります。

### 5. Debug 版のビルド

```bash
make debug
```

実行ファイルは `build/debug/build/Debug/Cudyne` に出力されます。

### 6. compile_commands.json の生成

```bash
make compdb
```

生成された `compile_commands.json` はリポジトリ直下にコピーされます
(`.gitignore` 済みなのでコミットされません)。

### ルート Makefile について

| ターゲット | 内容                                               |
| ---------- | -------------------------------------------------- |
| `release`  | Release 版を Ninja ジェネレータでビルド            |
| `debug`    | Debug 版を Ninja ジェネレータでビルド              |
| `compdb`   | `compile_commands.json` を生成（clangd/Neovim 用） |
| `clean`    | `build/` ディレクトリを削除                        |

`CMakeLists.txt` 側では `-DLINUX_BUILD=ON` を渡すことで、Conan
(`CMakeDeps`/`CMakeToolchain`) が生成したターゲットを利用してネイティブ Linux
バイナリをビルドします。
