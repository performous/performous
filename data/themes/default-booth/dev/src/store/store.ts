import { createStore, Store } from 'vuex';
import * as actions from './actions';
import * as mutations from './mutations';
import type { State } from '../types';

export default function initStore() : Store<State> {
    const state: State = {
        screen: 'folder',
        screenQuery: {},
        ...(JSON.parse(localStorage.getItem('performous_web_frontend_settings') ?? '{}')),
        language: {},
        database: [],
        folders: [],
        languages: [],
        playlist: [],
        timeout: 15,
    };

    return createStore({
        state,
        mutations,
        actions,
    });
}