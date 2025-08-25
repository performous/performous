import type { Commit, Dispatch } from 'vuex';
import type { ScreenQuery, Song, State, StringDict } from '../types';
import { getApi, postApi } from '../helpers/requests';
import debounce from '../helpers/debounce';

let songInterval: number;
let playlistInterval: number;
let refreshTimeout: number;

const debounceQuery = debounce(async (commit: Commit, query: string): Promise<void> => {
    let database: Song[];
    if (query) {
        database = await postApi('search', { query }, true);
    } else {
        database = await getApi('getDataBase.json');
    }
    commit('storeDatabase', database);
});

export function saveException({ commit }: { commit: Commit }, exception: Error): void {
    commit('storeException', exception);
}

export async function refreshDatabase({ state, commit, dispatch }: { state: State, commit: Commit, dispatch: Dispatch }, query: string = ''): Promise<void> {
    try {
        const database: Song[] = await getApi(`getDataBase.json`, query);
        commit('storeDatabase', database);
        if (state.offline) {
            commit('storeOfflineState', false);
            dispatch('setIntervals');
            dispatch('refreshPlaylist');
        }
        clearTimeout(refreshTimeout);
    } catch (exception) {
        clearTimeout(refreshTimeout);
        refreshTimeout = setTimeout(() => dispatch('refreshDatabase', query), 60000);
        dispatch('saveException', exception);
    }
}

export async function refreshLanguage({ commit }: { commit: Commit }): Promise<void> {
    const language: StringDict = await getApi('language');
    commit('storeLanguage', language);
}

export async function refreshPlaylist({ state, commit, dispatch }: { state: State, commit: Commit, dispatch: Dispatch }): Promise<void> {
    try {
        const playlist: Song[] = await getApi('getCurrentPlaylist.json');
        commit('storePlaylist', playlist);
        dispatch('refreshSong');
        const timeout = parseInt(await getApi('getplaylistTimeout', '', false)) ?? 15;
        commit('storeTimeout', timeout);
    }
    catch (exception) {
        commit('storeOfflineState', true);
        clearInterval(playlistInterval);

        clearTimeout(refreshTimeout);
        refreshTimeout = setTimeout(() => dispatch('refreshDatabase', state.screen === 'search' && state.screenQuery?.search ? state.screenQuery.search : ''), 60000);
        dispatch('saveException', exception);
    }
}

export async function refreshSong({ commit, dispatch, state }: { commit: Commit, dispatch: Dispatch, state: State }): Promise<void> {
    try {
        const song: Song|number|null = await getApi('getCurrentSong');
        if (typeof state.song !== typeof song) {
            dispatch('refreshPlaylist');
        }
        commit('storeSong', song);
    }
    catch (exception) {
        commit('storeSong', null);
        commit('storeOfflineState', true);
        clearInterval(songInterval);
        
        clearTimeout(refreshTimeout);
        refreshTimeout = setTimeout(() => dispatch('refreshDatabase', state.screen === 'search' && state.screenQuery?.search ? state.screenQuery.search : ''), 60000);
        dispatch('saveException', exception);
    }
}

export async function playSong({ dispatch }: { dispatch: Dispatch }, song: Song|JSON): Promise<void> {
    if ('songId' in song) {
        await postApi('play', song);
    } else {
        await postApi('add/play', song);
    }
    dispatch('refreshPlaylist');
}

export async function skipSong(): Promise<void> {
    await getApi('skip');
}

export async function addSong({ dispatch }: { dispatch: Dispatch }, song: Song): Promise<void> {
    await postApi('add', song);
    dispatch('refreshPlaylist');
}

export async function prioritizeSong({ dispatch }: { dispatch: Dispatch }, song: Song): Promise<void> {
    await postApi('add/priority', song);
    dispatch('refreshPlaylist');
}

export async function removeSong({ dispatch }: { dispatch: Dispatch }, song: JSON): Promise<void> {
    await postApi('remove', song);
    dispatch('refreshPlaylist');
}

export async function moveSong({ dispatch }: { dispatch: Dispatch }, song: JSON): Promise<void> {
    await postApi('setposition', song);
    dispatch('refreshPlaylist');
}

export function setScreen({ commit, dispatch }: { commit: Commit, dispatch: Dispatch }, screen: string): void {
    commit('storeScreen', screen);
    dispatch ('saveLocalStorage');
}

export function setScreenQuery({ commit, dispatch }: { commit: Commit, dispatch: Dispatch }, screenQuery: ScreenQuery): void {
    commit('storeScreenQuery', screenQuery);
    dispatch ('saveLocalStorage');
}

export function saveLocalStorage({ state }: { state: State }) {
    localStorage.setItem('performous_web_frontend_settings', JSON.stringify({
        screen: state.screen,
        screenQuery: state.screenQuery,
    }));
}

export function search({ commit, dispatch, state }: { commit: Commit, dispatch: Dispatch, state: State }, query: string): void {
    const newQuery = { ...state.screenQuery };
    newQuery.search = query;
    dispatch('setScreenQuery', newQuery);
    debounceQuery(commit, query);
}

export async function restorePlaylist({ commit, dispatch }: { commit: Commit, dispatch: Dispatch }): Promise<void> {
    const oldPlaylist = JSON.parse(sessionStorage.getItem('performous_playlist') ?? '[]');
    for (const song of oldPlaylist) {
        await postApi('add', song);
    }
    dispatch('refreshPlaylist');
    commit('storePreservedPlaylist', false);
}

export function removePlaylist({ commit }: { commit: Commit }): void {
    commit('storePreservedPlaylist', false);
}

export function setIntervals({ dispatch }: { dispatch: Dispatch }): void {
    songInterval = setInterval(() => {
        dispatch('refreshSong');
    }, 1000);
    
    playlistInterval = setInterval(() => {
        dispatch('refreshPlaylist');
    }, 10000);
}