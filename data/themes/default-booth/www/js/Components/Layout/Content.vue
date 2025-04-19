<script setup>
import Folders from './Content/Folders.vue';
import Search from './Content/Search.vue';
import Playlist from './Content/Playlist.vue';

const { watch } = Vue;
const { useStore } = Vuex;

const { content } = defineProps({
    content: {
        type: String,
        required: true,
    },
});

const store = useStore();

const map = {
    folders: Folders,
    search: Search,
    playlist: Playlist,
};
</script>
<template>
    <component :is="map[content] ?? map.folders" />
</template>