const WEEKDAY_ALIASES: Record<string, number> = {
    '0': 0,
    sun: 0,
    sunday: 0,
    '日': 0,
    '日曜': 0,
    '日曜日': 0,
    '1': 1,
    mon: 1,
    monday: 1,
    '月': 1,
    '月曜': 1,
    '月曜日': 1,
    '2': 2,
    tue: 2,
    tuesday: 2,
    '火': 2,
    '火曜': 2,
    '火曜日': 2,
    '3': 3,
    wed: 3,
    wednesday: 3,
    '水': 3,
    '水曜': 3,
    '水曜日': 3,
    '4': 4,
    thu: 4,
    thursday: 4,
    '木': 4,
    '木曜': 4,
    '木曜日': 4,
    '5': 5,
    fri: 5,
    friday: 5,
    '金': 5,
    '金曜': 5,
    '金曜日': 5,
    '6': 6,
    sat: 6,
    saturday: 6,
    '土': 6,
    '土曜': 6,
    '土曜日': 6,
};

const WEEKDAY_NAMES = ['sun', 'mon', 'tue', 'wed', 'thu', 'fri', 'sat'] as const;

export interface NormalisedSchedule {
    input_weekday: string;
    input_time: string;
    weekday: number;
    weekday_name: string;
    time: string;
    date_offset: number;
}

export function normalise_schedule(weekday_value: string, time_value: string): NormalisedSchedule {
    const input_weekday = weekday_value.trim();
    const input_time = time_value.trim();
    const weekday = parse_weekday(input_weekday);
    const { hours, minutes } = parse_extended_time(input_time);
    const date_offset = Math.floor(hours / 24);
    const normalised_weekday = (weekday + date_offset) % 7;
    const normalised_hour = hours % 24;

    return {
        input_weekday,
        input_time,
        weekday: normalised_weekday,
        weekday_name: WEEKDAY_NAMES[normalised_weekday],
        time: `${pad2(normalised_hour)}:${pad2(minutes)}`,
        date_offset,
    };
}

export function resolve_date_offset(value: unknown, automatic_offset: number): number {
    const raw = String(value ?? '').trim();
    if (!raw) {
        return automatic_offset;
    }

    const offset = Number(raw);
    if (!Number.isInteger(offset) || offset < 0) {
        throw new Error(`Date Offset must be a non-negative integer: ${raw}`);
    }
    return offset;
}

function parse_weekday(value: string): number {
    const weekday = WEEKDAY_ALIASES[value.toLowerCase()];
    if (weekday === undefined) {
        throw new Error(`unsupported weekday: ${value}`);
    }
    return weekday;
}

function parse_extended_time(value: string): { hours: number; minutes: number } {
    const match = value.match(/^(\d+):?(\d{2})(?::(\d{2}))?$/);
    if (!match) {
        throw new Error(`time must be HH:MM, HHMM, or HH:MM:00: ${value}`);
    }

    const hours = Number(match[1]);
    const minutes = Number(match[2]);
    const seconds = Number(match[3] ?? '0');
    if (
        !Number.isInteger(hours)
        || hours < 0
        || minutes < 0
        || minutes > 59
        || seconds !== 0
    ) {
        throw new Error(`invalid extended time: ${value}`);
    }
    return { hours, minutes };
}

function pad2(value: number): string {
    return String(value).padStart(2, '0');
}
