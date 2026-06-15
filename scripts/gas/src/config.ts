import {
    GOOGLE_DRIVE_FOLDER_ID,
    RADICC_SERVER_API_KEY,
    RADICC_SERVER_URL,
    SPREADSHEET_ID,
} from './env';

export interface AppConfig {
    spreadsheet_id: string;
    radicc_server_url: string;
    radicc_server_api_key_configured: boolean;
    google_drive_folder_configured: boolean;
}

export function get_app_config(): AppConfig {
    return {
        spreadsheet_id: get_spreadsheet_id(),
        radicc_server_url: get_radicc_server_url(),
        radicc_server_api_key_configured: get_radicc_server_api_key() !== '',
        google_drive_folder_configured: GOOGLE_DRIVE_FOLDER_ID.trim() !== '',
    };
}

export function get_spreadsheet_id(): string {
    const spreadsheet_id = SPREADSHEET_ID.trim();
    if (!spreadsheet_id) {
        throw new Error('SPREADSHEET_ID is empty');
    }
    return spreadsheet_id;
}

export function get_radicc_server_url(): string {
    const radicc_server_url = normalise_base_url(RADICC_SERVER_URL);
    if (!radicc_server_url) {
        throw new Error('RADICC_SERVER_URL is empty');
    }
    return radicc_server_url;
}

export function get_radicc_server_api_key(): string {
    return RADICC_SERVER_API_KEY.trim();
}

export function get_google_drive_folder_id(): string {
    return GOOGLE_DRIVE_FOLDER_ID.trim();
}

function normalise_base_url(value: string): string {
    const trimmed = value.trim();
    return trimmed.endsWith('/') ? trimmed.slice(0, -1) : trimmed;
}
