import { download_recording, RecordResponse, request_record } from '../radicc/api';
import { resolve_reservation, ReservationResolution } from './program_resolver';
import {
    ProgramReservation,
    get_first_program_reservation,
    update_last_recorded_start,
} from '../spreadsheet/programs';
import { append_record_log, RecordLogEntry } from '../spreadsheet/record_logs';
import {
    DriveFileInfo,
    is_drive_storage_configured,
    save_blob_to_drive,
} from '../storage/drive';

export interface RecordingResult {
    status: 'recorded' | 'skipped_duplicate';
    resolution: ReservationResolution;
    response: RecordResponse | null;
    drive_file: DriveFileInfo | null;
}

export function record_first_enabled_program(): RecordingResult {
    return record_reservation(get_first_program_reservation());
}

export function record_reservation(reservation: ProgramReservation): RecordingResult {
    const requested_at = now_text();
    let resolution: ReservationResolution | null = null;
    let response: RecordResponse | null = null;

    try {
        if (!reservation.enabled) {
            throw new Error('programme reservation is disabled');
        }

        resolution = resolve_reservation(reservation);
        if (!resolution.record_request || !resolution.resolution.selected) {
            throw new Error('no completed programme was found');
        }

        if (reservation.last_recorded_start === resolution.resolution.selected.ft) {
            append_record_log(build_duplicate_log(requested_at, resolution));
            return {
                status: 'skipped_duplicate',
                resolution,
                response: null,
                drive_file: null,
            };
        }

        response = request_record(resolution.record_request);
        const drive_file = is_drive_storage_configured()
            ? save_blob_to_drive(download_recording(response))
            : null;
        update_last_recorded_start(reservation.row_number, resolution.resolution.selected.ft);
        append_record_log(build_success_log(requested_at, resolution, response, drive_file));
        return {
            status: 'recorded',
            resolution,
            response,
            drive_file,
        };
    } catch (error) {
        append_record_log(build_failure_log(requested_at, reservation, resolution, response, error));
        throw error;
    }
}

function build_duplicate_log(requested_at: string, resolution: ReservationResolution): RecordLogEntry {
    const selected = resolution.resolution.selected;
    return {
        ...build_common_log(requested_at, resolution.reservation, resolution),
        resolved_start: selected?.ft ?? '',
        resolved_end: selected?.to ?? '',
        resolved_url: selected?.url ?? '',
        status: 'skipped_duplicate',
        message: 'Resolved programme start matches Last Recorded Start',
        job_id: '',
        download_url: '',
        output_file: '',
        drive_file_id: '',
        drive_file_url: '',
        response_json: '',
    };
}

function build_success_log(
    requested_at: string,
    resolution: ReservationResolution,
    response: RecordResponse,
    drive_file: DriveFileInfo | null
): RecordLogEntry {
    const selected = resolution.resolution.selected;
    return {
        ...build_common_log(requested_at, resolution.reservation, resolution),
        resolved_start: selected?.ft ?? '',
        resolved_end: selected?.to ?? '',
        resolved_url: selected?.url ?? '',
        status: response.status,
        message: '',
        job_id: response.job_id,
        download_url: response.download_url,
        output_file: response.output_file,
        drive_file_id: drive_file?.id ?? '',
        drive_file_url: drive_file?.url ?? '',
        response_json: JSON.stringify(response),
    };
}

function build_failure_log(
    requested_at: string,
    reservation: ProgramReservation,
    resolution: ReservationResolution | null,
    response: RecordResponse | null,
    error: unknown
): RecordLogEntry {
    const selected = resolution?.resolution.selected;
    return {
        ...build_common_log(requested_at, reservation, resolution),
        resolved_start: selected?.ft ?? '',
        resolved_end: selected?.to ?? '',
        resolved_url: selected?.url ?? '',
        status: 'error',
        message: error_message(error),
        job_id: response?.job_id ?? '',
        download_url: response?.download_url ?? '',
        output_file: response?.output_file ?? '',
        drive_file_id: '',
        drive_file_url: '',
        response_json: response ? JSON.stringify(response) : '',
    };
}

function build_common_log(
    requested_at: string,
    reservation: ProgramReservation,
    resolution: ReservationResolution | null
): Pick<RecordLogEntry, 'requested_at' | 'station_id' | 'title' | 'scheduled_weekday' | 'scheduled_time'> {
    return {
        requested_at,
        station_id: reservation.station_id,
        title: reservation.title,
        scheduled_weekday: resolution?.reservation.schedule.weekday_name ?? reservation.schedule.weekday_name,
        scheduled_time: resolution?.reservation.schedule.time ?? reservation.schedule.time,
    };
}

function now_text(): string {
    return Utilities.formatDate(new Date(), 'Asia/Tokyo', "yyyy-MM-dd'T'HH:mm:ssXXX");
}

function error_message(error: unknown): string {
    return error instanceof Error ? error.message : String(error);
}
