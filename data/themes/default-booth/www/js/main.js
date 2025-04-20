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
    await loadJS('./js/vueuse.shared.min.js');
    await loadJS('./js/vueuse.core.min.js');
    await loadJS('./js/constants.js');
    await loadJS('./js/helpers.js');
    await loadJS('./js/store.js');
    
    const { loadModule } = window['vue3-sfc-loader'];

    const options = {
        moduleCache: {
            vue: Vue,
            vuex: Vuex,
            '@vueuse/core': VueUse,
            '@constants': constants,
            '@helpers': helpers,
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

    const app = Vue.createApp({
        components: {
            'app': Vue.defineAsyncComponent(() => loadModule('./js/Components/App.vue', options)),
        },
        template: '<app />',
    }).use(store).mount('#app');

    const settings = JSON.parse(window.localStorage.getItem('performous_web_frontend_settings') ?? '{}');

    const query = [];

    query.push(`sort=${settings.sort || 'artist'}`);
    query.push(`order=${settings.descending ? 'descending' : 'ascending'}`)

    if (settings.screen === 'search' && settings.screenQuery?.search) {
        query.push(`query=${settings.screenQuery?.search}`);
    }

    app.$store.dispatch('refreshLanguage');
    app.$store.dispatch('refreshDatabase', query.join('&'));
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
