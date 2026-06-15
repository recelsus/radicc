# radicc GAS bridge

This directory contains a Google Apps Script project for programme reservations and Radiko URL resolution.

## What it does

- Initialises programme reservation and record log sheets
- Reads programme reservations from a Google Spreadsheet
- Resolves all weekly matches from `Station ID + Title`
- Classifies matches as completed, on-air, or future
- Selects the latest completed match

## Requirements

- `node >= 20`
- `npm`
- `clasp`
- A running `radicc-server`

## Environment

Create `scripts/gas/.env` from `.env.example`.

Required keys:

- `CLASP_SCRIPT_ID`
- `RADICC_SERVER_URL`
- `SPREADSHEET_ID`

Optional keys:

- `CLASP_ROOT_DIR` default: `dist`
- `CLASP_PROJECT_ID`
- `RADICC_SERVER_API_KEY`: Bearer API key sent to radicc-server
- `GOOGLE_DRIVE_FOLDER_ID`: Google Drive folder ID used to store recordings
## Spreadsheet format

Reservation sheet name: `programs`

Columns:

1. `Station ID *`
2. `Title *`
3. `Weekday *`
4. `Time *`
5. `Date Offset`
6. `Enabled *`
7. `Last Recorded Start`

URL resolution uses only `Station ID` and `Title`. `Weekday` and `Time` are reserved for triggers.  
`Enabled` accepts booleans or case-insensitive `TRUE` / `FALSE` strings. Blank values default to true,
and rows set to `FALSE` are excluded from URL resolution.

`Last Recorded Start` is managed by GAS for duplicate prevention. When it matches the resolved programme
start `ft`, no recording request is sent and the result is logged as `skipped_duplicate`. It is updated
only after a successful recording request.

Extended times such as `24:30` and `26:10` are supported. Values displayed by Google Sheets as
`24:30:00` are normalised to `24:30` by GAS.

- `Saturday 24:30` normalises to `Sunday 00:30` and automatically uses `Date Offset=1`.
- Set `Date Offset=1` explicitly for `Sunday 00:30` when it belongs to Saturday's broadcast day.
- Automatic offset calculation is used only when `Date Offset` is blank.

Record log sheet name: `record_logs`. It stores successful and failed recording requests, job IDs,
absolute download URLs, output filenames, Drive file IDs and URLs, and server response JSON.
Recordings are saved to Drive only when `GOOGLE_DRIVE_FOLDER_ID` is configured.

## Commands

```bash
cd scripts/gas
npm install
npm run push
```

`npm run push` fully regenerates `dist/` and replaces the remote content with `clasp push --force`. It does
not pull stale remote `.gs` files, so only files generated from the current `src/` tree are sent.

Run `npm run manifest` explicitly only when the existing remote manifest must be imported.

## Exported GAS functions

- `init()`: initialise `programs` and `record_logs`
- `main()`: record enabled reservations whose normalised weekday is yesterday
- `record_first_enabled_program()`: resolve the first enabled reservation, request recording, and store the result
- `record_previous_day_programs()`: record enabled reservations whose normalised weekday is yesterday
- `test_initialise_sheets()`: log sheet initialisation results
- `test_healthcheck()`: log the radicc-server `/health` response
- `test_log_config()`: log generated environment configuration
- `test_log_drive_folder()`: log access information for the configured Google Drive folder
- `test_log_previous_day_programs()`: log reservations that would be processed for yesterday
- `test_log_program_reservations()`: log programme reservations
- `test_normalise_schedule_examples()`: log weekday, extended-time, and date-offset conversions
- `test_record_first_program()`: send a real recording request for the first enabled reservation
- `test_resolve_first_program()`: resolve and log the first reservation

Manual test functions live in `src/tests/manual.ts` and require no arguments.

`main()` is intended for a daily trigger. It uses the normalised actual weekday, so `Saturday 24:30`
is treated as `Sunday 00:30` and recorded by Monday's `main()` execution.
