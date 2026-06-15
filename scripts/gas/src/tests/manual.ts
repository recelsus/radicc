import { get_app_config } from '../config';
import { initialise_sheets } from '../app/initialise';
import { resolve_reservation } from '../app/program_resolver';
import {
    get_enabled_program_reservations,
    get_first_program_reservation,
    get_program_reservations,
} from '../spreadsheet/programs';
import { normalise_schedule, resolve_date_offset } from '../schedule/normalise';
import { healthcheck } from '../radicc/api';
import { record_first_enabled_program } from '../app/recording';
import { get_previous_day_reservations } from '../app/daily_recording';
import { get_drive_folder_info } from '../storage/drive';

export function test_log_config(): void {
    console.log(JSON.stringify({
        scope: 'test.config',
        config: get_app_config(),
    }));
}

export function test_initialise_sheets(): void {
    console.log(JSON.stringify({
        scope: 'test.initialise_sheets',
        result: initialise_sheets(),
    }));
}

export function test_healthcheck(): void {
    console.log(JSON.stringify({
        scope: 'test.healthcheck',
        result: healthcheck(),
    }));
}

export function test_log_drive_folder(): void {
    console.log(JSON.stringify({
        scope: 'test.drive_folder',
        result: get_drive_folder_info(),
    }));
}

export function test_log_program_reservations(): void {
    const reservations = get_program_reservations();
    const enabled_reservations = get_enabled_program_reservations();
    console.log(JSON.stringify({
        scope: 'test.program_reservations',
        count: reservations.length,
        enabled_count: enabled_reservations.length,
        reservations,
    }));
}

export function test_resolve_first_program(): void {
    const reservation = get_first_program_reservation();
    console.log(JSON.stringify({
        scope: 'test.resolve_first_program',
        result: resolve_reservation(reservation),
    }));
}

export function test_record_first_program(): void {
    console.log(JSON.stringify({
        scope: 'test.record_first_program',
        result: record_first_enabled_program(),
    }));
}

export function test_log_previous_day_programs(): void {
    const reservations = get_previous_day_reservations();
    console.log(JSON.stringify({
        scope: 'test.previous_day_programs',
        count: reservations.length,
        reservations,
    }));
}

export function test_normalise_schedule_examples(): void {
    const sunday_schedule = normalise_schedule('日曜', '00:30');
    const saturday_schedule = normalise_schedule('土曜', '24:30');
    const sunday_offset = resolve_date_offset(1, sunday_schedule.date_offset);
    const saturday_offset = resolve_date_offset('', saturday_schedule.date_offset);
    const examples = [
        {
            label: 'Sunday 00:30 with explicit date offset',
            schedule: sunday_schedule,
            effective_date_offset: sunday_offset,
        },
        {
            label: 'Saturday 24:30 with automatic date offset',
            schedule: saturday_schedule,
            effective_date_offset: saturday_offset,
        },
        {
            label: 'Saturday 24:30:00 formatted by Google Sheets',
            schedule: normalise_schedule('土曜', '24:30:00'),
        },
        normalise_schedule('金曜', '26:10'),
    ];
    console.log(JSON.stringify({
        scope: 'test.normalise_schedule_examples',
        examples,
        equivalent: sunday_schedule.weekday === saturday_schedule.weekday
            && sunday_schedule.time === saturday_schedule.time
            && sunday_offset === saturday_offset,
    }));
}
