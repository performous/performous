<script setup lang="ts">
import Folders from './Content/Folders.vue';
import Search from './Content/Search.vue';
import Playlist from './Content/Playlist.vue';
import Languages from './Content/Languages.vue';
import type { Component } from 'vue';

const { content } = defineProps({
    content: {
        type: String,
        required: true,
    },
});

const map: {
    [key: string]: Component
} = {
    folders: Folders,
    languages: Languages,
    search: Search,
    playlist: Playlist,
};
</script>
<template>
    <component :is="map[content] ?? map.folders" />
</template>

<style lang="css">
.categories {
    display: flex;
    flex-wrap: wrap;
    gap: 1rem;
    justify-content: center;
    padding-bottom: 1rem;
}

.categories button {
    padding: 1rem;
    color: var(--color-primary-fg);
    background-color: var(--color-primary-bg);
    width: 20rem;
    cursor: pointer;
}

.categories button:hover, .categories button:focus {
    background-color: var(--color-primary-bg-hover);
}

.categories button .heading {
    font-size: 1.25rem;
    font-weight: 700;
}
</style>