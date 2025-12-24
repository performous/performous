<script setup lang="ts">
import { computed, onBeforeUnmount, ref, watch } from 'vue';
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

const songs = computed(() => [...store.state.database]);

const songLength = computed(() => songs.value.length);
const prevLength = ref(songLength.value);
const renderLimit = ref(100);

onBeforeUnmount(() => {
    store.dispatch('refreshDatabase');
});

watch(songLength, (newLength) => {
    if (prevLength.value !== newLength) {
        renderLimit.value = 0;
    }
    prevLength.value = newLength;
});
</script>
<template>
<div>
    <div class="heading">{{ $store.state.language.search }}</div>
    <div class="search-input"><input type="text" v-model="search" /></div>
    <SongList :songs v-model="renderLimit" />
</div>
</template>

<style lang="css">
.search-input {
    display: flex;
    justify-content: stretch;
    padding: 0 1rem 1rem;
}

.search-input > input {
    flex: 1 0 0;
    border-radius: 1000rem;
    padding: 1rem;
    color: var(--color-general-fg);
    background-color: var(--color-general-bg);
    border: 2px solid var(--color-general-fg);
}
</style>