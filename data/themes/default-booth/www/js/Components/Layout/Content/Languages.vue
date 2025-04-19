<script setup>
import { computed } from 'vue';
import { useStore } from 'vuex';
import { getLanguageName } from '@helpers';
import Category from './Category.vue';
import Cover from '../../Common/Cover.vue';
import ContentBar from './ContentBar.vue';
import Flag from '../../Common/Flag.vue';

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
    const list = {};
    languages.value.forEach((language) => {
        list[language] = [];
    });

    store.state.database.forEach((song) => {
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
        <div class="folderlist">
            <div
                v-for="language in languages"
            >
                <button
                    type="button"
                    @click="category = language"
                >
                    <Flag :language />
                    <p class="category">{{ language }}</p>
                    <p>{{ $store.state.language.available_songs }}: {{ languageList[language].length }}</p>
                </button>
            </div>
        </div>
    </div>
</template>