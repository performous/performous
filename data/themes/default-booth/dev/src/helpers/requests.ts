import type { Dataable } from "../types";

export async function getApi(request: string, query: string = '', asJSON: boolean = true): Promise<any> {
    const prefix = import.meta.env.DEV ? `http://localhost:${import.meta.env.VITE_SERVER_PORT}` : '.';
    const res = await fetch(`${prefix}/api/${request}${query ? `?${query}` : ''}`, import.meta.env.DEV ? { mode: 'no-cors' } : {});
    if (asJSON) {
        return await res.json();
    }
    return await res.text();
}

export async function postApi(request: string, payload: Dataable, asJSON: boolean = false): Promise<any> {
    const prefix = import.meta.env.DEV ? `http://localhost:${import.meta.env.VITE_SERVER_PORT}` : '.';
    const res = await fetch(`${prefix}/api/${request}`, {
        method: 'post',
        body: JSON.stringify(payload),
        headers: {
            'Content-Type': 'application/json; charset=utf-8',
        },
        ...(import.meta.env.DEV ? { mode: 'no-cors' } : {})
    });
    if (asJSON) {
        return await res.json();
    }
    return await res.text();
}
