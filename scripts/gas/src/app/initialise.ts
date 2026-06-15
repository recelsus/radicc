import { init_programs_sheet } from '../spreadsheet/programs';
import { init_record_logs_sheet } from '../spreadsheet/record_logs';

export interface InitialisedSheets {
    programs: string;
    record_logs: string;
}

export function initialise_sheets(): InitialisedSheets {
    const programs = init_programs_sheet();
    const record_logs = init_record_logs_sheet();
    return {
        programs: programs.getName(),
        record_logs: record_logs.getName(),
    };
}
