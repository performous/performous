<script setup lang="ts">
import { computed } from 'vue';
import { useStore } from 'vuex';
import Category from './Category.vue';
import ContentBar from './ContentBar.vue';
import Folder from '../../Common/Folder.vue';
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

const folders = computed(() => store.state.folders);

const folderList = computed(() => {
    const list: {[key: string]: Song[]} = {};
    folders.value.forEach((folder: string) => {
        list[folder] = [];
    });

    store.state.database.forEach((song: Song) => {
        list[song.Path].push(song);
    });

    return list;
});
</script>
<template>
    <Category v-if="category" :songs="folderList[category]" v-model="category" />
    <div v-else>
        <ContentBar />
        <div class="categories">
            <div
                v-for="folder in folders"
            >
                <button
                    type="button"
                    @click="category = folder"
                >
                    <Folder :folder="folder" />
                    <p class="heading">{{ folder }}</p>
                    <p>{{ $translate('available_songs') }}: {{ folderList[folder].length }}</p>
                </button>
            </div>
        </div>
    </div>
</template>