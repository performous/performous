const files = {};

function duration(time, withHour = false) {
    const seconds = Math.floor(time);
    return `${withHour ? `${Math.floor(seconds / 3600)}:` : ''}${`${Math.floor(seconds / 60) % 60}`.padStart(2, '0')}:${`${seconds % 60}`.padStart(2, '0')}`;
}

async function loadJS(file) {
    if (files[file]) {
        return await files[file];
    }

    const js = document.createElement('script');
    js.type = 'text/javascript';
    js.src = file;

    files[file] = new Promise((resolve) => {
        js.addEventListener('readystatechange', resolve);
        js.addEventListener('load', resolve);
    });

    document.head.appendChild(js);

    await files[file];
}

const init = async () => {
    document.removeEventListener('DOMContentLoaded', init);
    window.removeEventListener('load', init);

    await loadJS('./js/vue.global.js');
    await loadJS('./js/vuex.global.js');
    await loadJS('./js/vue3-sfc-loader.js');
    
    const { loadModule } = window['vue3-sfc-loader'];

    const options = {
        moduleCache: {
            vue: Vue
        },
        async getFile(url) {
            const res = await fetch(url);
            if ( !res.ok )
                throw Object.assign(new Error(res.statusText + ' ' + url), { res });
            return {
                getContentData: asBinary => asBinary ? res.arrayBuffer() : res.text(),
            }
        },
        addStyle(textContent) {
            const style = Object.assign(document.createElement('style'), { textContent });
            const ref = document.head.getElementsByTagName('style')[0] || null;
            document.head.insertBefore(style, ref);
        },
    };

    async function getApi(request, query = '') {
        const res = await fetch(`./api/${request}${query ? `?${query}` : ''}`);
        return await res.json();
    }

    const store = Vuex.createStore({
        state () {
            return {
                screen: 'folder',
                screenQuery: '',
                ...(JSON.parse(window.localStorage.getItem('performous_web_frontend_settings') ?? '{}')),
                language: {},
                database: [],
                folders: [],
                playlist: [],
                timeout: 10,
            };
        },
        mutations: {
            storeDatabase(state, value) {
                state.database = value;
                state.folders = value.reduce((list, song) => {
                    const folder = song.Path;
                    if (!list.includes(folder)) {
                        list.push(folder);
                    }
                    return list;
                }, []).sort();
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
        },
        actions: {
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
        }
    });

    const app = Vue.createApp({
        components: {
            'app': Vue.defineAsyncComponent(() => loadModule('./js/Components/App.vue', options)),
        },
        template: '<app />',
    }).use(store).mount('#app');

    app.$store.dispatch('refreshLanguage');
    app.$store.dispatch('refreshDatabase');
    app.$store.dispatch('refreshPlaylist');
    app.$store.dispatch('refreshSong');

    setInterval(() => {
        app.$store.dispatch('refreshPlaylist');
    }, 10000);

    setInterval(() => {
        app.$store.dispatch('refreshSong');
    }, 1000);
}

if (document.readyState === 'complete') {
    init();
} else {
    document.addEventListener('DOMContentLoaded', init);
    window.addEventListener('load', init);
}
