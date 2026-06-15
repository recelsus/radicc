import { get_radicc_server_api_key, get_radicc_server_url } from '../config';

export interface RecordRequest {
    url: string;
    date_offset?: number;
}

export interface HealthcheckResponse {
    status: string;
}

export interface RecordResponse {
    status: string;
    job_id: string;
    download_url: string;
    filepath: string;
    output_file: string;
    title: string;
    start_time: string;
    end_time: string;
}

export function healthcheck(): HealthcheckResponse {
    return fetch_json('/health', { method: 'get' }) as unknown as HealthcheckResponse;
}

export function request_record(payload: RecordRequest): RecordResponse {
    const response = fetch_json('/record', {
        method: 'post',
        contentType: 'application/json',
        payload: JSON.stringify(payload),
    }) as unknown as RecordResponse;
    return {
        ...response,
        download_url: resolve_server_url(response.download_url),
    };
}

export function download_recording(response: RecordResponse): GoogleAppsScript.Base.Blob {
    const download_response = fetch_response(response.download_url, { method: 'get' });
    return download_response.getBlob().setName(response.output_file);
}

function resolve_server_url(path_or_url: string): string {
    if (!path_or_url || /^https?:\/\//i.test(path_or_url)) {
        return path_or_url;
    }
    const path = path_or_url.startsWith('/') ? path_or_url : `/${path_or_url}`;
    return `${get_radicc_server_url()}${path}`;
}

function fetch_json(path: string, options: GoogleAppsScript.URL_Fetch.URLFetchRequestOptions): Record<string, unknown> {
    const response = fetch_response(`${get_radicc_server_url()}${path}`, options);
    const text = response.getContentText();
    if (!text) {
        return {};
    }
    return JSON.parse(text) as Record<string, unknown>;
}

function fetch_response(
    url: string,
    options: GoogleAppsScript.URL_Fetch.URLFetchRequestOptions
): GoogleAppsScript.URL_Fetch.HTTPResponse {
    const api_key = get_radicc_server_api_key();
    const headers = {
        ...options.headers,
        ...(api_key ? { Authorization: `Bearer ${api_key}` } : {}),
    };
    const response = UrlFetchApp.fetch(url, {
        muteHttpExceptions: true,
        ...options,
        headers,
    });
    const status = response.getResponseCode();
    if (status < 200 || status >= 300) {
        throw new Error(`radicc-server request failed: ${status} ${response.getContentText()}`);
    }
    return response;
}
