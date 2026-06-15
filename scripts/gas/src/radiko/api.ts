export interface TsEntry {
    url: string;
    sid: string;
    title: string;
    pfm: string;
    date: string;
    time: string;
    dur_sec: number;
}

export interface WeeklyProgramEntry {
    sid: string;
    title: string;
    pfm: string;
    ft: string;
    to: string;
    date: string;
    time: string;
    dur_sec: number;
    url: string;
}

export type ProgramState = 'completed' | 'on_air' | 'future';

export interface ResolvedProgramEntry extends WeeklyProgramEntry {
    state: ProgramState;
}

export interface ProgramResolution {
    station_id: string;
    title: string;
    resolved_at: string;
    match_count: number;
    completed_match_count: number;
    on_air_match_count: number;
    future_match_count: number;
    matches: ResolvedProgramEntry[];
    selected: ResolvedProgramEntry | null;
}

export interface WeeklyXmlDebugInfo {
    sid: string;
    url: string;
    raw_prog_count: number;
    parsed_prog_count: number;
    dates: string[];
    body_preview: string;
}

export interface EndpointProbeResult {
    label: string;
    url: string;
    status: number;
    elapsed_ms: number;
    content_type: string;
    set_cookie: boolean;
    body_preview: string;
}

export interface EndpointProbeReport {
    sid: string;
    area_id: string;
    today: string;
    yesterday: string;
    results: EndpointProbeResult[];
}

interface RadikoProgram {
    ft: string;
    to: string;
    dur: string;
    sid: string;
    title: string;
    pfm: string;
    ft_date: Date;
    to_date: Date;
}

export function build_station_weekly_url(sid: string): string {
    return `https://radiko.jp/v3/program/station/weekly/${sid}.xml`;
}

export function build_timeshift_url(sid: string, ft: string): string {
    return `https://radiko.jp/#!/ts/${sid}/${ft}`;
}

export function yyyymmdd(date: Date): string {
    return Utilities.formatDate(date, 'JST', 'yyyyMMdd');
}

export function resolve_lookup_date(days_ago: number): string {
    const target = new Date();
    target.setDate(target.getDate() - days_ago);
    return yyyymmdd(target);
}

export function api_entry(date_yyyymmdd: string, sid: string, title: string): TsEntry | null {
    if (!sid || !sid.trim()) {
        throw new Error('api_entry requires sid');
    }
    if (!title || !title.trim()) {
        throw new Error('api_entry requires title');
    }
    if (!date_yyyymmdd || !date_yyyymmdd.trim()) {
        throw new Error('api_entry requires date_yyyymmdd');
    }

    console.log(JSON.stringify({
        scope: 'radiko.api_entry.start',
        date_yyyymmdd,
        sid,
        title,
    }));

    const programs = get_station_programs_by_weekly(sid);
    console.log(JSON.stringify({
        scope: 'radiko.api_entry.programs_loaded',
        date_yyyymmdd,
        sid,
        title,
        program_count: programs.length,
        sample_titles: programs.slice(0, 5).map((program) => program.title),
    }));

    const hits = programs.filter((program) => program.title.trim() === title.trim());
    console.log(JSON.stringify({
        scope: 'radiko.api_entry.title_hits',
        date_yyyymmdd,
        sid,
        title,
        hit_count: hits.length,
        hits: hits.slice(0, 5).map((program) => ({
            title: program.title,
            ft: program.ft,
            to: program.to,
        })),
    }));

    if (hits.length === 0) {
        return null;
    }

    const cutoff = build_lookup_cutoff(date_yyyymmdd);
    const past_hits = hits.filter((program) => program.to_date.getTime() <= cutoff.getTime());
    console.log(JSON.stringify({
        scope: 'radiko.api_entry.past_hits',
        date_yyyymmdd,
        sid,
        title,
        cutoff: Utilities.formatDate(cutoff, 'JST', "yyyy-MM-dd'T'HH:mm:ss"),
        hit_count: past_hits.length,
        hits: past_hits.slice(0, 5).map((program) => ({
            title: program.title,
            ft: program.ft,
            to: program.to,
        })),
    }));

    if (past_hits.length === 0) {
        return null;
    }

    past_hits.sort((left, right) => right.to_date.getTime() - left.to_date.getTime());
    return to_ts_entry(past_hits[0]);
}

export function resolve_program_by_title(sid: string, title: string, now = new Date()): ProgramResolution {
    if (!sid || !sid.trim()) {
        throw new Error('resolve_program_by_title requires sid');
    }
    if (!title || !title.trim()) {
        throw new Error('resolve_program_by_title requires title');
    }

    const matches = get_station_programs_by_weekly(sid)
        .filter((program) => program.title.trim() === title.trim())
        .map((program) => to_resolved_program_entry(program, now))
        .sort((left, right) => left.ft.localeCompare(right.ft));
    const completed = matches.filter((program) => program.state === 'completed');
    const selected = completed.length > 0 ? completed[completed.length - 1] : null;

    return {
        station_id: sid,
        title,
        resolved_at: Utilities.formatDate(now, 'Asia/Tokyo', "yyyy-MM-dd'T'HH:mm:ssXXX"),
        match_count: matches.length,
        completed_match_count: completed.length,
        on_air_match_count: matches.filter((program) => program.state === 'on_air').length,
        future_match_count: matches.filter((program) => program.state === 'future').length,
        matches,
        selected,
    };
}

export function dump_weekly_programs(sid: string): WeeklyProgramEntry[] {
    if (!sid || !sid.trim()) {
        throw new Error('dump_weekly_programs requires sid');
    }

    console.log(JSON.stringify({
        scope: 'radiko.dump_weekly_programs.start',
        sid,
    }));

    const programs = get_station_programs_by_weekly(sid)
        .sort((left, right) => left.ft.localeCompare(right.ft))
        .map((program) => to_weekly_program_entry(program));

    console.log(JSON.stringify({
        scope: 'radiko.dump_weekly_programs.result',
        sid,
        program_count: programs.length,
        sample: programs.slice(0, 5),
    }));

    return programs;
}

export function debug_weekly_xml(sid: string): WeeklyXmlDebugInfo {
    if (!sid || !sid.trim()) {
        throw new Error('debug_weekly_xml requires sid');
    }

    const url = build_station_weekly_url(sid);
    const body = fetch_weekly_xml_text(sid);
    const parsed_programs = parse_programs_xml(body, sid);
    const dates = extract_weekly_dates(body);
    const info = {
        sid,
        url,
        raw_prog_count: count_prog_tags(body),
        parsed_prog_count: parsed_programs.length,
        dates,
        body_preview: preview_text(body),
    };

    console.log(JSON.stringify({
        scope: 'radiko.debug_weekly_xml.result',
        ...info,
    }));

    return info;
}

export function debug_program_endpoints(sid: string, area_id: string): EndpointProbeReport {
    if (!sid || !sid.trim()) {
        throw new Error('debug_program_endpoints requires sid');
    }
    if (!area_id || !area_id.trim()) {
        throw new Error('debug_program_endpoints requires area_id');
    }

    const today = yyyymmdd(new Date());
    const yesterday_date = new Date();
    yesterday_date.setDate(yesterday_date.getDate() - 1);
    const yesterday = yyyymmdd(yesterday_date);

    const date_today_url = build_station_date_url(today, sid);
    const date_yesterday_url = build_station_date_url(yesterday, sid);
    const weekly_url = build_station_weekly_url(sid);
    const today_alias_url = `https://radiko.jp/v3/program/station/today/${sid}.xml`;
    const yesterday_alias_url = `https://radiko.jp/v3/program/station/yesterday/${sid}.xml`;
    const area_date_today_url = `https://radiko.jp/v3/program/date/${today}/${area_id}.xml`;
    const area_date_yesterday_url = `https://radiko.jp/v3/program/date/${yesterday}/${area_id}.xml`;
    const area_today_url = `https://radiko.jp/v3/program/today/${area_id}.xml`;
    const area_yesterday_url = `https://radiko.jp/v3/program/yesterday/${area_id}.xml`;
    const results: EndpointProbeResult[] = [];

    results.push(probe_endpoint('date_today_cold', date_today_url));
    results.push(probe_endpoint('date_today_retry', date_today_url));

    const weekly_response = probe_endpoint_with_cookie('weekly_warmup', weekly_url);
    results.push(weekly_response.result);

    results.push(probe_endpoint('date_today_after_weekly', date_today_url));
    results.push(probe_endpoint('date_yesterday_after_weekly', date_yesterday_url));

    if (weekly_response.cookie) {
        results.push(probe_endpoint('date_today_with_weekly_cookie', date_today_url, weekly_response.cookie));
        results.push(probe_endpoint('date_yesterday_with_weekly_cookie', date_yesterday_url, weekly_response.cookie));
    }

    results.push(probe_endpoint('today_alias', today_alias_url));
    results.push(probe_endpoint('yesterday_alias', yesterday_alias_url));
    results.push(probe_endpoint('area_date_today', area_date_today_url));
    results.push(probe_endpoint('area_date_yesterday', area_date_yesterday_url));
    results.push(probe_endpoint('area_today', area_today_url));
    results.push(probe_endpoint('area_yesterday', area_yesterday_url));

    const report = {
        sid,
        area_id,
        today,
        yesterday,
        results,
    };
    console.log(JSON.stringify({
        scope: 'radiko.debug_program_endpoints.result',
        ...report,
    }));
    return report;
}

export function build_station_date_url(date_yyyymmdd: string, sid: string): string {
    return `https://radiko.jp/v3/program/station/date/${date_yyyymmdd}/${sid}.xml`;
}

function get_station_programs_by_weekly(sid: string): RadikoProgram[] {
    const body = fetch_weekly_xml_text(sid);
    const url = build_station_weekly_url(sid);
    const programs = parse_programs_xml(body, sid);
    console.log(JSON.stringify({
        scope: 'radiko.fetch.parsed',
        sid,
        url,
        program_count: programs.length,
    }));
    return programs;
}

function fetch_weekly_xml_text(sid: string): string {
    const url = build_station_weekly_url(sid);
    console.log(JSON.stringify({
        scope: 'radiko.fetch.start',
        sid,
        url,
    }));

    const response = UrlFetchApp.fetch(url, {
        muteHttpExceptions: true,
    });
    const status = response.getResponseCode();
    const body = response.getContentText();
    console.log(JSON.stringify({
        scope: 'radiko.fetch.response',
        sid,
        url,
        status,
        body_preview: preview_text(body),
    }));

    if (status !== 200) {
        throw new Error(
            `radiko fetch failed: status=${status} sid=${sid} `
            + `url=${url} body=${preview_text(body)}`
        );
    }

    return body;
}

function parse_programs_xml(xml_text: string, sid: string): RadikoProgram[] {
    const doc = XmlService.parse(xml_text);
    const root = doc.getRootElement();
    const station = find_station_element(root, sid);
    if (!station) {
        return [];
    }

    const prog_nodes = collect_prog_nodes(station);
    return prog_nodes.map((prog) => parse_program(prog, sid));
}

function parse_program(node: GoogleAppsScript.XML_Service.Element, sid: string): RadikoProgram {
    const ft = get_attr(node, 'ft');
    const to = get_attr(node, 'to');
    return {
        ft,
        to,
        dur: get_attr(node, 'dur'),
        sid,
        title: node.getChildText('title') ?? '',
        pfm: node.getChildText('pfm') ?? '',
        ft_date: parse_yyyymmddhhmmss(ft),
        to_date: parse_yyyymmddhhmmss(to),
    };
}

function parse_yyyymmddhhmmss(value: string): Date {
    if (value.length !== 14) {
        return new Date();
    }
    const year = Number(value.slice(0, 4));
    const month = Number(value.slice(4, 6)) - 1;
    const day = Number(value.slice(6, 8));
    const hour = Number(value.slice(8, 10));
    const minute = Number(value.slice(10, 12));
    const second = Number(value.slice(12, 14));
    return new Date(year, month, day, hour, minute, second);
}

function to_ts_entry(program: RadikoProgram): TsEntry {
    return {
        url: build_timeshift_url(program.sid, program.ft),
        sid: program.sid,
        title: program.title,
        pfm: program.pfm,
        date: Utilities.formatDate(program.ft_date, 'JST', 'yyyy-MM-dd'),
        time: `${Utilities.formatDate(program.ft_date, 'JST', 'HH:mm')}-${Utilities.formatDate(program.to_date, 'JST', 'HH:mm')}`,
        dur_sec: Number(program.dur || '0') || 0,
    };
}

function to_weekly_program_entry(program: RadikoProgram): WeeklyProgramEntry {
    return {
        sid: program.sid,
        title: program.title,
        pfm: program.pfm,
        ft: program.ft,
        to: program.to,
        date: Utilities.formatDate(program.ft_date, 'JST', 'yyyy-MM-dd'),
        time: `${Utilities.formatDate(program.ft_date, 'JST', 'HH:mm')}-${Utilities.formatDate(program.to_date, 'JST', 'HH:mm')}`,
        dur_sec: Number(program.dur || '0') || 0,
        url: build_timeshift_url(program.sid, program.ft),
    };
}

function to_resolved_program_entry(program: RadikoProgram, now: Date): ResolvedProgramEntry {
    let state: ProgramState = 'on_air';
    if (program.to_date.getTime() <= now.getTime()) {
        state = 'completed';
    } else if (program.ft_date.getTime() > now.getTime()) {
        state = 'future';
    }

    return {
        ...to_weekly_program_entry(program),
        state,
    };
}

function get_attr(node: GoogleAppsScript.XML_Service.Element, name: string): string {
    return node.getAttribute(name)?.getValue() ?? '';
}

function build_lookup_cutoff(date_yyyymmdd: string): Date {
    if (!/^\d{8}$/.test(date_yyyymmdd)) {
        return new Date();
    }

    const year = Number(date_yyyymmdd.slice(0, 4));
    const month = Number(date_yyyymmdd.slice(4, 6)) - 1;
    const day = Number(date_yyyymmdd.slice(6, 8));
    return new Date(year, month, day, 23, 59, 59);
}

function preview_text(value: string): string {
    return value.replace(/\s+/g, ' ').trim().slice(0, 300);
}

function count_prog_tags(xml_text: string): number {
    const matches = xml_text.match(/<prog\b/g);
    return matches ? matches.length : 0;
}

function extract_weekly_dates(xml_text: string): string[] {
    const matches = Array.from(xml_text.matchAll(/<date>(\d{8})<\/date>/g));
    return matches.map((match) => match[1]);
}

function probe_endpoint(label: string, url: string, cookie?: string): EndpointProbeResult {
    return probe_endpoint_with_cookie(label, url, cookie).result;
}

function probe_endpoint_with_cookie(
    label: string,
    url: string,
    cookie?: string
): { result: EndpointProbeResult; cookie: string } {
    const headers: Record<string, string> = {
        Accept: 'application/xml,text/xml,*/*',
        Referer: 'https://radiko.jp/',
    };
    if (cookie) {
        headers.Cookie = cookie;
    }

    const started_at = Date.now();
    const response = UrlFetchApp.fetch(url, {
        headers,
        muteHttpExceptions: true,
    });
    const elapsed_ms = Date.now() - started_at;
    const response_headers = response.getAllHeaders();
    const set_cookie = find_header(response_headers, 'set-cookie');
    const result = {
        label,
        url,
        status: response.getResponseCode(),
        elapsed_ms,
        content_type: find_header(response_headers, 'content-type'),
        set_cookie: Boolean(set_cookie),
        body_preview: preview_text(response.getContentText()),
    };

    console.log(JSON.stringify({
        scope: 'radiko.endpoint_probe',
        ...result,
    }));
    return {
        result,
        cookie: normalise_cookie(set_cookie),
    };
}

function find_header(headers: object, wanted_name: string): string {
    for (const [name, value] of Object.entries(headers)) {
        if (name.toLowerCase() !== wanted_name.toLowerCase()) {
            continue;
        }
        return Array.isArray(value) ? value.join('; ') : String(value ?? '');
    }
    return '';
}

function normalise_cookie(set_cookie: string): string {
    if (!set_cookie) {
        return '';
    }
    return set_cookie
        .split(/,(?=[^;,]+=)/)
        .map((cookie) => cookie.split(';', 1)[0].trim())
        .filter(Boolean)
        .join('; ');
}

function find_station_element(
    root: GoogleAppsScript.XML_Service.Element,
    sid: string
): GoogleAppsScript.XML_Service.Element | null {
    const stations = root.getChild('stations');
    if (!stations) {
        return null;
    }

    const station_nodes = stations.getChildren('station');
    for (const station of station_nodes) {
        const station_id = station.getAttribute('id')?.getValue() ?? '';
        if (station_id === sid) {
            return station;
        }
    }

    return station_nodes.length > 0 ? station_nodes[0] : null;
}

function collect_prog_nodes(
    node: GoogleAppsScript.XML_Service.Element
): GoogleAppsScript.XML_Service.Element[] {
    const results: GoogleAppsScript.XML_Service.Element[] = [];

    if (node.getName() === 'prog') {
        results.push(node);
    }

    for (const child of node.getChildren()) {
        results.push(...collect_prog_nodes(child));
    }

    return results;
}
