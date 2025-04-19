
const store = (() => {
    async function getApi(request, query = '') {
        const res = await fetch(`./api/${request}${query ? `?${query}` : ''}`);
        return await res.json();
    }

    async function postApi(request, payload, asJSON = false) {
        const res = await fetch(`./api/${request}`, {
            method: 'post',
            body: JSON.stringify(payload),
            headers: {
                'Content-Type': 'application/json; charset=utf-8',
            },
        });
        if (asJSON) {
            return await res.json();
        }
        return await res.text();
    }

    function debounce(callback, timeout = 300) {
        let timeoutId;
        return (...args) => {
            clearTimeout(timeoutId);
            timeoutId = setTimeout(() => callback(...args), timeout);
        }
    }

    const debounceQuery = debounce(async (commit, query) => {
        let database;
        if (query) {
            database = await postApi('search', {
                query,
            }, true);
        } else {
            database = await getApi('getDataBase.json');
        }
        commit('storeDatabase', database);
    });

    function state() {
        return {
            screen: 'folder',
            screenQuery: {},
            ...(JSON.parse(window.localStorage.getItem('performous_web_frontend_settings') ?? '{}')),
            language: {},
            database: [],
            folders: [],
            languages: [],
            playlist: [],
            timeout: 10,
        };
    }
    const mutations = {
        storeDatabase(state, value) {
            state.database = value;
            const folders = [];
            const languages = [];

            value.forEach((song) => {
                const folder = song.Path;
                if (!folders.includes(folder)) {
                    folders.push(folder);
                }
                if (song.Language && song.Language.trim()) {
                    song.Language.split(',').forEach((lang) => {
                        const l = helpers.getLanguageName(lang);
                        if (!languages.includes(l)) {
                            languages.push(l);
                        }
                    });
                }
            });

            state.folders = folders.sort();
            state.languages = languages.sort();
        },
        storeLanguage(state, value) {
            state.language = value;
        },
        storePlaylist(state, value) {
            state.playlist = value;
        },
        storeSong(state, value) {
            state.song = value;
        },
        setTimeout(state, value) {
            state.timeout = value;
        },
        setScreen(state, value) {
            state.screen = value;
        },
        setScreenQuery(state, value) {
            state.screenQuery = value;
        },
    };

    const actions = {
        async refreshDatabase({ commit }, query = '') {
            const database = await getApi(`getDataBase.json${query ? `?${query}` : ''}`);
            commit('storeDatabase', database);
        },
        async refreshLanguage({ commit }) {
            const language = await getApi('language');
            commit('storeLanguage', language);
        },
        async refreshPlaylist({ commit, dispatch }) {
            const playlist = await getApi('getCurrentPlaylist.json');
            commit('storePlaylist', playlist);
            dispatch('refreshSong');
            const res = await fetch('api/getplaylistTimeout');
            const timeoutStr = await res.text();
            const timeout = parseInt(timeoutStr);
            commit('setTimeout', timeout);
        },
        async refreshSong({ commit, dispatch, state }) {
            const song = await getApi('getCurrentSong');
            if ((state.song && !song) || (!state.song && song)) {
                dispatch('refreshPlaylist');
            }
            commit('storeSong', song);
        },
        async playSong({ dispatch }, song) {
            if (song.songId) {
                await fetch('./api/play', {
                    method: 'post',
                    body: JSON.stringify(song),
                    headers: {
                        'Content-Type': 'application/json; charset=utf-8',
                    },
                });
            } else {
                await fetch('./api/add/play', {
                    method: 'post',
                    body: JSON.stringify(song),
                    headers: {
                        'Content-Type': 'application/json; charset=utf-8',
                    },
                });
            }
            dispatch('refreshPlaylist');
        },
        async addSong({ dispatch }, song) {
            await fetch('./api/add', {
                method: 'post',
                body: JSON.stringify(song),
                headers: {
                    'Content-Type': 'application/json; charset=utf-8',
                },
            });
            dispatch('refreshPlaylist');
        },
        async prioritizeSong({ dispatch }, song) {
            await fetch('./api/add/priority', {
                method: 'post',
                body: JSON.stringify(song),
                headers: {
                    'Content-Type': 'application/json; charset=utf-8',
                },
            });
            dispatch('refreshPlaylist');
        },
        async removeSong({ dispatch }, song) {
            await fetch('./api/remove', {
                method: 'post',
                body: JSON.stringify(song),
                headers: {
                    'Content-Type': 'application/json; charset=utf-8',
                },
            });
            dispatch('refreshPlaylist');
        },
        async moveSong({ dispatch }, song) {
            await fetch('./api/setposition', {
                method: 'post',
                body: JSON.stringify(song),
                headers: {
                    'Content-Type': 'application/json; charset=utf-8',
                },
            });
            dispatch('refreshPlaylist');
        },
        setScreen({ commit, dispatch }, screen) {
            commit('setScreen', screen);
            dispatch('saveLocalStorage');
        },
        setScreenQuery({ commit, dispatch }, query) {
            commit('setScreenQuery', query);
            dispatch('saveLocalStorage');
        },
        saveLocalStorage({ state }) {
            window.localStorage.setItem('performous_web_frontend_settings', JSON.stringify({
                screen: state.screen,
                screenQuery: state.screenQuery,
            }));
        },
        search({ commit, dispatch }, query) {
            const newQuery = { ...store.state.screenQuery };
            newQuery.search = query;
            dispatch('setScreenQuery', newQuery);
            debounceQuery(commit, query);
        },
    };

    const storeExport = Vuex.createStore({
        state,
        mutations,
        actions,
    });
    return storeExport;
})();
