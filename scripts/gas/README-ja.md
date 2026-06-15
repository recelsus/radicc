# radicc GAS bridge

このディレクトリには、予約番組の管理と Radiko URL 解決を行う Google Apps Script 一式を置きます。

## 機能

- 予約番組シートと取得ログシートを初期化
- 予約番組シートから番組情報を取得
- `Station ID + Title` から週次番組表を検索
- 一致した全番組を未来・放送中・終了済みに分類
- 終了済み番組のうち最新回を選択

## 前提

- `node >= 20`
- `npm`
- `clasp`
- 起動済みの `radicc-server`

## 環境変数

`scripts/gas/.env.example` をもとに `scripts/gas/.env` を作成します。

必須:

- `CLASP_SCRIPT_ID`
- `RADICC_SERVER_URL`
- `SPREADSHEET_ID`

任意:

- `CLASP_ROOT_DIR` 既定値: `dist`
- `CLASP_PROJECT_ID`
- `RADICC_SERVER_API_KEY`: radicc-serverへ送信するBearer APIキー
- `GOOGLE_DRIVE_FOLDER_ID`: 録音ファイルを保存するGoogle DriveフォルダID
## スプレッドシート形式

予約番組シート名は `programs` です。

列:

1. `Station ID *`
2. `Title *`
3. `Weekday *`
4. `Time *`
5. `Date Offset`
6. `Enabled *`
7. `Last Recorded Start`

URL検索には `Station ID` と `Title` を使用します。`Weekday` と `Time` はトリガー用です。  
`Enabled` はbooleanまたは大文字小文字を問わない文字列の `TRUE` / `FALSE` を受け付けます。
空欄は有効として扱い、`FALSE` の行はURL解決対象から除外します。

`Last Recorded Start` はGASが管理する重複防止用の列です。解決した番組の開始時刻 `ft` と一致する場合は
録音要求を送らず、`skipped_duplicate` としてログへ記録します。録音成功時のみ値を更新します。

`Time` は `24:30` や `26:10` の拡張時刻表記を許容します。Google Sheetsによって
`24:30:00` のように表示された場合も、GAS側で `24:30` として扱います。

- `土曜 24:30` は `日曜 00:30` に正規化され、`Date Offset=1` が自動設定されます。
- `日曜 00:30` を土曜深夜扱いにする場合は、`Date Offset` に `1` を明示します。
- `Date Offset` が空の場合のみ、拡張時刻から自動算出します。

取得ログシート名は `record_logs` です。初期化時にログ用ヘッダーを作成します。
録音要求の成功・失敗、job ID、絶対形式のダウンロードURL、出力ファイル名、DriveファイルID・URL、
サーバー応答JSONを記録します。`GOOGLE_DRIVE_FOLDER_ID` が設定されている場合のみDriveへ保存します。

## コマンド

```bash
cd scripts/gas
npm install
npm run push
```

`npm run push` は `dist/` を完全に再生成し、`clasp push --force` で現在の内容へ置き換えます。
リモート上の旧 `.gs` ファイルはpullせず、現在の `src/` から生成したファイルだけを送信します。

既存のリモートmanifestを取り込む場合のみ、明示的に `npm run manifest` を実行してください。

## 公開関数

- `init()`: `programs` と `record_logs` を初期化
- `main()`: 前日の実曜日に該当する有効予約を録音し、結果をログへ保存
- `record_first_enabled_program()`: 最初の有効予約を解決し、録音要求を送信して結果をログへ保存
- `record_previous_day_programs()`: 前日の実曜日に該当する有効予約を録音
- `test_initialise_sheets()`: シート初期化結果をログ出力
- `test_healthcheck()`: radicc-serverの `/health` 応答をログ出力
- `test_log_config()`: 環境設定をログ出力
- `test_log_drive_folder()`: 保存先Google Driveフォルダへのアクセス結果をログ出力
- `test_log_previous_day_programs()`: 前日分として処理される予約一覧をログ出力
- `test_log_program_reservations()`: 予約番組一覧をログ出力
- `test_normalise_schedule_examples()`: 曜日・拡張時刻・Date Offsetの変換例をログ出力
- `test_record_first_program()`: 最初の有効予約へ実際に録音要求を送信
- `test_resolve_first_program()`: 予約番組リストの先頭行を使い、URL解決結果をログ出力

手動確認関数は `src/tests/manual.ts` にあります。GASエディタから引数なしで実行できます。

`main()` は日次トリガー用です。曜日判定には正規化後の実曜日を使用するため、`土曜 24:30` は
`日曜 00:30` として扱われ、月曜日の `main()` 実行時に録音されます。
