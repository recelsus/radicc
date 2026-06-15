import { ensure_sheet_with_headers, open_spreadsheet } from './common';
import {
    NormalisedSchedule,
    normalise_schedule,
    resolve_date_offset,
} from '../schedule/normalise';

export const PROGRAMS_SHEET_NAME = 'programs';
export const PROGRAMS_HEADERS = [
    'Station ID *',
    'Title *',
    'Weekday *',
    'Time *',
    'Date Offset',
    'Enabled *',
    'Last Recorded Start',
] as const;

const LEGACY_PROGRAMS_HEADERS = [
    'Station ID *',
    'Title *',
    'Weekday *',
    'Time *',
    'Enabled *',
] as const;

export interface ProgramReservation {
    row_number: number;
    station_id: string;
    title: string;
    weekday: string;
    time: string;
    schedule: NormalisedSchedule;
    date_offset: number;
    enabled: boolean;
    last_recorded_start: string;
}

export function init_programs_sheet(): GoogleAppsScript.Spreadsheet.Sheet {
    migrate_legacy_programs_sheet();
    return ensure_sheet_with_headers(PROGRAMS_SHEET_NAME, PROGRAMS_HEADERS);
}

export function get_program_reservations(): ProgramReservation[] {
    const sheet = init_programs_sheet();
    const range = sheet.getDataRange();
    const values = range.getValues();
    const display_values = range.getDisplayValues();
    return values
        .slice(1)
        .map((row, index) => parse_program_reservation(index + 2, row, display_values[index + 1]))
        .filter((row): row is ProgramReservation => row !== null);
}

export function get_enabled_program_reservations(): ProgramReservation[] {
    return get_program_reservations().filter((reservation) => reservation.enabled);
}

export function get_first_program_reservation(): ProgramReservation {
    const reservation = get_enabled_program_reservations()[0];
    if (!reservation) {
        throw new Error(`no enabled programme reservations found in ${PROGRAMS_SHEET_NAME}`);
    }
    return reservation;
}

function parse_program_reservation(
    row_number: number,
    row: unknown[],
    display_row: string[]
): ProgramReservation | null {
    const station_id = display_row[0].trim();
    const title = display_row[1].trim();
    if (!station_id || !title) {
        return null;
    }

    const weekday = display_row[2].trim();
    const time = format_sheet_time(row[3], display_row[3]);
    const schedule = normalise_schedule(weekday, time);
    return {
        row_number,
        station_id,
        title,
        weekday,
        time,
        schedule,
        date_offset: resolve_date_offset(row[4], schedule.date_offset),
        enabled: parse_enabled(row[5]),
        last_recorded_start: display_row[6]?.trim() ?? '',
    };
}

export function update_last_recorded_start(row_number: number, start: string): void {
    init_programs_sheet().getRange(row_number, 7).setValue(start);
}

function parse_enabled(value: unknown): boolean {
    if (typeof value === 'boolean') {
        return value;
    }
    const normalised = String(value ?? '').trim().toLowerCase();
    if (normalised === '' || normalised === 'true') {
        return true;
    }
    if (normalised === 'false') {
        return false;
    }
    throw new Error(`Enabled must be TRUE or FALSE: ${String(value ?? '')}`);
}

function format_sheet_time(value: unknown, display_value: string): string {
    const display_time = display_value.trim();
    if (typeof value !== 'number') {
        return display_time;
    }

    const total_minutes = Math.round(value * 24 * 60);
    if (total_minutes < 0) {
        throw new Error(`Time must not be negative: ${display_time}`);
    }
    const hours = Math.floor(total_minutes / 60);
    const minutes = total_minutes % 60;
    return `${String(hours).padStart(2, '0')}:${String(minutes).padStart(2, '0')}`;
}

function migrate_legacy_programs_sheet(): void {
    const sheet = open_spreadsheet().getSheetByName(PROGRAMS_SHEET_NAME);
    if (!sheet) {
        return;
    }

    const headers = sheet.getRange(1, 1, 1, LEGACY_PROGRAMS_HEADERS.length).getDisplayValues()[0];
    const is_legacy = LEGACY_PROGRAMS_HEADERS.every((header, index) => headers[index].trim() === header);
    if (is_legacy) {
        sheet.insertColumnBefore(5);
    }
}
