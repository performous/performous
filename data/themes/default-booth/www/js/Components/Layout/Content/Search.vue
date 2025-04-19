<script setup>
import { computed, onBeforeUnmount } from 'vue';
import { useStore } from 'vuex';
import SongList from '../../Common/SongList.vue';

const store = useStore();

const search = computed({
    get() {
        if (!store.state.screenQuery?.search) {
            return '';
        }
        return store.state.screenQuery.search;
    },
    set(value) {
        store.dispatch('search', value);
    },
});

const songs = computed(() => store.state.database);

onBeforeUnmount(() => {
    store.dispatch('refreshDatabase');
});
</script>
<template>
<div class="search">
    <div class="header">{{ $store.state.language.search }}</div>
    <div class="input"><input type="text" v-model="search" /></div>
    <SongList :songs />
</div>
</template>