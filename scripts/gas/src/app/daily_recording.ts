import { record_reservation, RecordingResult } from './recording';
import {
    get_enabled_program_reservations,
    ProgramReservation,
} from '../spreadsheet/programs';

const DAY_MS = 24 * 60 * 60 * 1000;

export interface DailyRecordingFailure {
    row_number: number;
    station_id: string;
    title: string;
    error: string;
}

export interface DailyRecordingSummary {
    target_date: string;
    target_weekday: number;
    target_weekday_name: string;
    reservation_count: number;
    recorded_count: number;
    skipped_duplicate_count: number;
    failure_count: number;
    results: RecordingResult[];
    failures: DailyRecordingFailure[];
}

export function record_previous_day_programs(now = new Date()): DailyRecordingSummary {
    const target = previous_day(now);
    const reservations = get_previous_day_reservations(now);
    const results: RecordingResult[] = [];
    const failures: DailyRecordingFailure[] = [];

    for (const reservation of reservations) {
        try {
            results.push(record_reservation(reservation));
        } catch (error) {
            failures.push({
                row_number: reservation.row_number,
                station_id: reservation.station_id,
                title: reservation.title,
                error: error_message(error),
            });
        }
    }

    return {
        target_date: target.date,
        target_weekday: target.weekday,
        target_weekday_name: target.weekday_name,
        reservation_count: reservations.length,
        recorded_count: results.filter((result) => result.status === 'recorded').length,
        skipped_duplicate_count: results.filter((result) => result.status === 'skipped_duplicate').length,
        failure_count: failures.length,
        results,
        failures,
    };
}

export function get_previous_day_reservations(now = new Date()): ProgramReservation[] {
    const target_weekday = previous_day(now).weekday;
    return get_enabled_program_reservations()
        .filter((reservation) => reservation.schedule.weekday === target_weekday);
}

function previous_day(now: Date): { date: string; weekday: number; weekday_name: string } {
    const target = new Date(now.getTime() - DAY_MS);
    const date = Utilities.formatDate(target, 'Asia/Tokyo', 'yyyyMMdd');
    const weekday = weekday_from_yyyymmdd(date);
    const weekday_name = ['sun', 'mon', 'tue', 'wed', 'thu', 'fri', 'sat'][weekday];
    return { date, weekday, weekday_name };
}

function weekday_from_yyyymmdd(value: string): number {
    const year = Number(value.slice(0, 4));
    const month = Number(value.slice(4, 6));
    const day = Number(value.slice(6, 8));
    return new Date(Date.UTC(year, month - 1, day)).getUTCDay();
}

function error_message(error: unknown): string {
    return error instanceof Error ? error.message : String(error);
}
