import { get_spreadsheet_id } from '../config';

export function open_spreadsheet(): GoogleAppsScript.Spreadsheet.Spreadsheet {
    return SpreadsheetApp.openById(get_spreadsheet_id());
}

export function ensure_sheet_with_headers(
    sheet_name: string,
    headers: readonly string[]
): GoogleAppsScript.Spreadsheet.Sheet {
    const spreadsheet = open_spreadsheet();
    const sheet = spreadsheet.getSheetByName(sheet_name) ?? spreadsheet.insertSheet(sheet_name);

    if (sheet.getMaxColumns() < headers.length) {
        sheet.insertColumnsAfter(sheet.getMaxColumns(), headers.length - sheet.getMaxColumns());
    }

    if (!has_expected_headers(sheet, headers)) {
        if (looks_like_header(sheet, headers.length)) {
            sheet.getRange(1, 1, 1, headers.length).setValues([Array.from(headers)]);
        } else if (is_first_row_empty(sheet, headers.length)) {
            sheet.getRange(1, 1, 1, headers.length).setValues([Array.from(headers)]);
        } else {
            sheet.insertRowsBefore(1, 1);
            sheet.getRange(1, 1, 1, headers.length).setValues([Array.from(headers)]);
        }
    }

    sheet.setFrozenRows(1);
    return sheet;
}

function has_expected_headers(
    sheet: GoogleAppsScript.Spreadsheet.Sheet,
    headers: readonly string[]
): boolean {
    const row = sheet.getRange(1, 1, 1, headers.length).getValues()[0];
    return headers.every((header, index) => String(row[index] ?? '').trim() === header);
}

function is_first_row_empty(sheet: GoogleAppsScript.Spreadsheet.Sheet, width: number): boolean {
    const row = sheet.getRange(1, 1, 1, width).getValues()[0];
    return row.every((value) => String(value ?? '').trim() === '');
}

function looks_like_header(sheet: GoogleAppsScript.Spreadsheet.Sheet, width: number): boolean {
    const row = sheet.getRange(1, 1, 1, width).getValues()[0]
        .map((value) => String(value ?? '').trim().replace(/\s*\*$/, '').toLowerCase());
    return row.includes('station id') && row.includes('title');
}
