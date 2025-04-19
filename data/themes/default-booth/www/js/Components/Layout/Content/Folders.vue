<script setup>
import Category from './Folders/Category.vue';
import Cover from '../../Common/Cover.vue';

const { computed, ref } = Vue;
const { useStore } = Vuex;

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
    const folderList = {};
    folders.value.forEach((folder) => {
        folderList[folder] = [];
    });

    store.state.database.forEach((song) => {
        folderList[song.Path].push(song);
    });

    return folderList;
});
</script>
<template>
    <Category v-if="category" :songs="folderList[category]" v-model="category" />
    <div class="folderlist" v-else>
        <div
            v-for="folder in folders"
        >
            <button
                type="button"
                @click="category = folder"
            >
                <Cover :file="`folders/${folder}`" :alt="folder" />
                <p class="category">{{ folder }}</p>
                <p>{{ $store.state.language.available_songs }}: {{ folderList[folder].length }}</p>
            </button>
        </div>
    </div>
</template>