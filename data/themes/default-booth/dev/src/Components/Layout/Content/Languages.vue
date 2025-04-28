<script setup lang="ts">
import { computed } from 'vue';
import { useStore } from 'vuex';
import Category from './Category.vue';
import ContentBar from './ContentBar.vue';
import Language from '../../Common/Language.vue';
import getLanguageName from '../../../helpers/getLanguageName';
import type { Song } from '../../../types';

const store = useStore();

const category = computed({
    get() {
        if (!store.state.screenQuery?.category) {
            return null;
        }
        return store.state.screenQuery.category;
    },
    set(value) {
        const newQuery = { ...store.state.screenQuery };
        newQuery.category = value;
        store.dispatch('setScreenQuery', newQuery);
    },
});

const languages = computed(() => store.state.languages);

const languageList = computed(() => {
    const list: {[key: string]: Song[]} = {};
    languages.value.forEach((language: string) => {
        list[language] = [];
    });

    store.state.database.forEach((song: Song) => {
        if (song.Language && song.Language.trim()) {
            song.Language.split(',').forEach((lang) => {
                const l = getLanguageName(lang);
                list[l].push(song);
            });
        }
    });

    return list;
});
</script>
<template>
    <Category v-if="category" :songs="languageList[category]" v-model="category" />
    <div v-else>
        <ContentBar />
        <div class="categories">
            <div
                v-for="language in languages"
            >
                <button
                    type="button"
                    @click="category = language"
                >
                    <Language :language />
                    <p class="heading">{{ language }}</p>
                    <p>{{ $translate('available_songs') }}: {{ languageList[language].length }}</p>
                </button>
            </div>
        </div>
    </div>
</template>