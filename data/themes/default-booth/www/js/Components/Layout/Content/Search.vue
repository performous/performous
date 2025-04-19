<script setup>
import SongList from '../../Common/SongList.vue';

const { computed, ref } = Vue;
const { useStore } = Vuex;

const store = useStore();

const search = computed({
    get() {
        if (!store.state.screenQuery?.search) {
            return '';
        }
        return store.state.screenQuery.search;
    },
    set(value) {
        const newQuery = { ...store.state.screenQuery };
        newQuery.search = value;
        store.dispatch('setScreenQuery', newQuery);
    },
});

const songs = computed(() => {
    if (!search.value) {
        return store.state.database;
    }
    const query = search.value.toLowerCase().replaceAll(/[^a-z0-9]/g, '');
    // Search regardless of order
    return store.state.database.filter((song) => {
        const title = song.Title.toLowerCase().replaceAll(/[^a-z0-9]/g, '');
        const artist = song.Artist.toLowerCase().replaceAll(/[^a-z0-9]/g, '');
        return `${title}${artist}`.includes(query) || `${artist}${title}`.includes(query);
    });
});
</script>
<template>
<div class="search">
    <div class="header">{{ $store.state.language.search }}</div>
    <div class="input"><input type="text" v-model="search" /></div>
    <SongList :songs />
</div>
</template>