# Radicc

## Features

- `.toml`ファイルから自動で設定を読み込み。
- 録音ファイルにアーティスト名やアルバム名のメタデータを追加可能。

## Depends

- C++20に対応したコンパイラ（例: clang++,g++）。
- `ffmpeg`

## Install 

1. **Clone:**

   ```bash
   git clone <repository-url>
   cd radicc 
   ```

2. **CMake:**

   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

   `build`ディレクトリ内に`radicc`が作成されます.

## Config

### .env

`.env`ファイルにRadikoのログイン情報を保存します。  
1. `XDG_CONFIG_HOME/radicc`
2. `~/.config/radicc` 
の`.env`または`env`ファイルを自動で探します。

```bash
RADIKO_USER=youremail@yourdomain.ch
RADIKO_PASS=yourpassword
OUTPUT_DIR="$HOME/Radiko"
```

### radicc.toml

`radico.toml`ファイルでテンプレート設定を記述できます.
`.env`ファイルと同じディレクトリで検索されます。

```toml
[example]
station = "JORF"
day = "Sunday"
time = "0300"
duration = "30"
output = "Zatsudan-{yyyyMMdd}-{HHmm}.m4a"
personality = "artist"
organize = "ZatsudanRadio"
```

具体的な内容は`radicc.toml.example`

## Usage

Radiko Recorderはコマンドライン引数を使うか,設定ファイルを使用して録音を行います。

### コマンドライン

- `--target` または `-t`: `radico.toml`ファイル内のセクションを指定.
- `--url` または `-u`: Radikoの番組URLを指定することで`放送局`と`開始時間`を指定できます.
- `--duration` または `-d`: 放送時間を分単位で指定します。
- `--output` または `-o`: 出力ファイル名を指定します。

### 使用例

1. **`radico.toml`を使用して録音:**

   ```bash
   ./radicc --target example 
   # or
   ./radicc --t example 
   ```

   `radico.toml`の`example`セクションに基づいて録音を行います。

2. **URLを使って録音:**

   ```bash
   ./radicc --url 'https://radiko.jp/#!/ts/JORF/20241013003000' --duration 30 --output "example.m4a"
   ```

   指定されたURLから30分間の放送を録音し,`output.m4a`という名前で保存します。

3. **カスタム録音時間と出力ファイルを指定して録音:**

   ```bash
   ./radicc --target example --duration 45 --output "example_special.m4a"
   ```

   `example`セクションに基づきつつ,例えば放送時間の変更や特別番組でのタイトル変更に対応します。

### 環境変数

環境変数を使ってRadikoにログインできます。

- `RADIKO_USER`: Radikoのメールアドレス。
- `RADIKO_PASS`: Radikoのパスワード。

これらは,`.env`や`env`ファイルに保存しておくことができます。

#### Comment

ほぼ車輪の再発明に他なりませんが, 

TOMLファイルでの番組の設定  
曜日指定から最も近い日付lを特定して録音
また24:30のような特殊な表記を冠している番組でもファイル名を前日分に変更が可能  
アーティスト名とアルバム名のみメタタグの追加 (現在はtomlからのみ指定可能)

を追加しています.  
CMakeだけは上手くいってるかわからないので問題があれば教えてください.
