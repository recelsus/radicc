import { ensure_sheet_with_headers } from './common';

export const RECORD_LOGS_SHEET_NAME = 'record_logs';
export const RECORD_LOGS_HEADERS = [
    'Requested At',
    'Station ID',
    'Title',
    'Scheduled Weekday',
    'Scheduled Time',
    'Resolved Start',
    'Resolved End',
    'Resolved URL',
    'Status',
    'Message',
    'Job ID',
    'Download URL',
    'Output File',
    'Drive File ID',
    'Drive File URL',
    'Response JSON',
] as const;

export interface RecordLogEntry {
    requested_at: string;
    station_id: string;
    title: string;
    scheduled_weekday: string;
    scheduled_time: string;
    resolved_start: string;
    resolved_end: string;
    resolved_url: string;
    status: string;
    message: string;
    job_id: string;
    download_url: string;
    output_file: string;
    drive_file_id: string;
    drive_file_url: string;
    response_json: string;
}

export function init_record_logs_sheet(): GoogleAppsScript.Spreadsheet.Sheet {
    return ensure_sheet_with_headers(RECORD_LOGS_SHEET_NAME, RECORD_LOGS_HEADERS);
}

export function append_record_log(entry: RecordLogEntry): void {
    init_record_logs_sheet().appendRow([
        entry.requested_at,
        entry.station_id,
        entry.title,
        entry.scheduled_weekday,
        entry.scheduled_time,
        entry.resolved_start,
        entry.resolved_end,
        entry.resolved_url,
        entry.status,
        entry.message,
        entry.job_id,
        entry.download_url,
        entry.output_file,
        entry.drive_file_id,
        entry.drive_file_url,
        entry.response_json,
    ]);
}
