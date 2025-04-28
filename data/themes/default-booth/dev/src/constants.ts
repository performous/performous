import type { StringDict } from "./types";

export const remapLanguages: StringDict = {};
export const remapFolders: StringDict = {};

async function loadData(file: string, dict: StringDict): Promise<void> {
    try {
        const res = await fetch(`./js/data/${file}.json`);
        const data = await res.json();
        Object.assign(dict, data);
    } catch (_e) {
        // Just skip
    }
};

export async function initData(): Promise<void> {
    await Promise.all([
        loadData('remapLanguages', remapLanguages),
        loadData('remapFolders', remapFolders),
    ]);
}