import { initialise_sheets } from './app/initialise';
import { record_first_enabled_program } from './app/recording';
import { record_previous_day_programs } from './app/daily_recording';
import {
    test_initialise_sheets,
    test_healthcheck,
    test_log_config,
    test_log_drive_folder,
    test_log_previous_day_programs,
    test_log_program_reservations,
    test_normalise_schedule_examples,
    test_record_first_program,
    test_resolve_first_program,
} from './tests/manual';

function init(): void {
    console.log(JSON.stringify({
        scope: 'main.init',
        result: initialise_sheets(),
    }));
}

function main(): void {
    console.log(JSON.stringify({
        scope: 'main.record_previous_day_programs',
        result: record_previous_day_programs(),
    }));
}

declare const global: any;
global.init = init;
global.main = main;
global.record_first_enabled_program = record_first_enabled_program;
global.record_previous_day_programs = record_previous_day_programs;
global.test_healthcheck = test_healthcheck;
global.test_initialise_sheets = test_initialise_sheets;
global.test_log_config = test_log_config;
global.test_log_drive_folder = test_log_drive_folder;
global.test_log_previous_day_programs = test_log_previous_day_programs;
global.test_log_program_reservations = test_log_program_reservations;
global.test_normalise_schedule_examples = test_normalise_schedule_examples;
global.test_record_first_program = test_record_first_program;
global.test_resolve_first_program = test_resolve_first_program;
