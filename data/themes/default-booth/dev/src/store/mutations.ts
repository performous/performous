import getLanguageName from "../helpers/getLanguageName";
import type { ScreenQuery, Song, State, StringDict } from "../types";

export function storeDatabase(state: State, value: Song[]): void {
    state.database = value;
    const folders: string[] = [];
    const languages: string[] = [];

    value.forEach((song: Song) => {
        const folder: string = song.Path;
        if (!folders.includes(folder)) {
            folders.push(folder);
        }
        if (song.Language?.trim()) {
            song.Language.split(',').forEach((lang: string) => {
                const realLang: string = getLanguageName(lang);
                if (!languages.includes(realLang)) {
                    languages.push(realLang);
                }
            });
        }
    });

    state.folders = folders.toSorted((a, b) => a.localeCompare(b));
    state.languages = languages.toSorted((a, b) => a.localeCompare(b));
}

export function storeLanguage(state: State, value: StringDict): void {
    state.language = value;
}

export function storePlaylist(state: State, value: Song[]): void {
    state.playlist = value;
}

export function storeSong(state: State, value: Song|number|null): void {
    state.song = value;
}

export function storeTimeout(state: State, value: number): void {
    state.timeout = value;
}

export function storeScreen(state: State, value: string): void {
    state.screen = value;
}

export function storeScreenQuery(state: State, value: ScreenQuery): void {
    state.screenQuery = value;
}

export function storeOfflineState(state: State, value: boolean): void {
    state.offline = value;
    if (value && state.playlist.length > 0) {
        sessionStorage.setItem('performous_playlist', JSON.stringify(state.playlist));
        state.hasPreservedPlaylist = true;
    }
}

export function storePreservedPlaylist(state: State, value: boolean): void {
    state.hasPreservedPlaylist = value;
    if (!value) {
        sessionStorage.removeItem('performous_playlist');
    }
}