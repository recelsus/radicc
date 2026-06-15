import { get_google_drive_folder_id } from '../config';

export interface DriveFileInfo {
    id: string;
    name: string;
    url: string;
}

export interface DriveFolderInfo {
    id: string;
    name: string;
    url: string;
}

export function get_drive_folder_info(): DriveFolderInfo {
    const folder = get_drive_folder();
    return {
        id: folder.getId(),
        name: folder.getName(),
        url: folder.getUrl(),
    };
}

export function is_drive_storage_configured(): boolean {
    return get_google_drive_folder_id() !== '';
}

export function save_blob_to_drive(blob: GoogleAppsScript.Base.Blob): DriveFileInfo {
    const folder = get_drive_folder();
    const file = folder.createFile(blob);
    return {
        id: file.getId(),
        name: file.getName(),
        url: file.getUrl(),
    };
}

function get_drive_folder(): GoogleAppsScript.Drive.Folder {
    const folder_id = get_google_drive_folder_id();
    if (!folder_id) {
        throw new Error('GOOGLE_DRIVE_FOLDER_ID is empty');
    }
    return DriveApp.getFolderById(folder_id);
}
