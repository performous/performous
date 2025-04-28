import { Store } from 'vuex';

declare module 'vue' {
    interface ComponentCustomProperties {
        $store: Store<State>
        $translate: (key: string) => string
    }
}
