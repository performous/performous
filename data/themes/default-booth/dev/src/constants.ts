import type { StringDict } from "./types";

export const remapLanguages: StringDict = {};
export const remapFlags: StringDict = {};
export const remapFolders: StringDict = {};

async function loadData(file: string, dict: StringDict): Promise<void> {
    const res = await fetch(`./js/data/${file}.json`);
    if (res.ok) {
        const data = await res.json();
        Object.assign(dict, data);
    }
};

export async function initData(): Promise<void> {
    await Promise.all([
        loadData('remapLanguages', remapLanguages),
        loadData('remapFlags', remapFlags),
        loadData('remapFolders', remapFolders),
    ]);
}