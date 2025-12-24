import { createApp } from 'vue';
import { Store } from 'vuex';
import './style.css'
import App from './App.vue'
import { initData } from './constants'
import type { LocalStorageData, State } from './types';
import initStore from './store/store';
import translatePlugin from './plugins/translatePlugin';

async function init(): Promise<void> {
    await initData();

    if (import.meta.env.DEV) {
        const styleCSS = document.head.querySelector('link[href$="css/style.css"]');
        if (styleCSS) {
            const viteDev = document.head.querySelectorAll('[data-vite-dev-id]');
            viteDev.forEach((devElement) => {
                document.head.insertBefore(devElement, styleCSS);
            });
        }
    }
    
    const store: Store<State> = initStore();

    createApp(App)
    .use(store)
    .use(translatePlugin)
    .mount('#app');

    const { screen, screenQuery }: LocalStorageData = JSON.parse(window.localStorage.getItem('performous_web_frontend_settings') ?? '{}');

    const query: string[] = [];
    
    query.push(`sort=${screenQuery?.sort ? screenQuery.sort : 'artist'}`);
    query.push(`order=${screenQuery?.descending ? 'descending' : 'ascending'}`)

    if (screen === 'search' && screenQuery?.search) {
        query.push(`query=${screenQuery.search}`);
    }

    store.dispatch('refreshLanguage');
    store.dispatch('refreshDatabase', query.join('&'));
    store.dispatch('refreshPlaylist');
    store.dispatch('refreshSong');

    store.dispatch('setIntervals');
}

init();