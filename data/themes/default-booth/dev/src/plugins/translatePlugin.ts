import type { App } from 'vue';

export default {
    install(app: App) {
        app.config.globalProperties.$translate = (key: string): string => app.config.globalProperties.$store.state.language[key] ?? key;
    }
}