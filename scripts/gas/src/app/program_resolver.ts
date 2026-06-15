import { ProgramResolution, resolve_program_by_title } from '../radiko/api';
import { ProgramReservation } from '../spreadsheet/programs';

export interface ReservationResolution {
    reservation: ProgramReservation;
    resolution: ProgramResolution;
    record_request: {
        url: string;
        date_offset: number;
    } | null;
}

export function resolve_reservation(reservation: ProgramReservation): ReservationResolution {
    const resolution = resolve_program_by_title(reservation.station_id, reservation.title);
    return {
        reservation,
        resolution,
        record_request: resolution.selected ? {
            url: resolution.selected.url,
            date_offset: reservation.date_offset,
        } : null,
    };
}
