<script setup>
import Folders from './Content/Folders.vue';
import Search from './Content/Search.vue';
import Playlist from './Content/Playlist.vue';
import Languages from './Content/Languages.vue';

const { content } = defineProps({
    content: {
        type: String,
        required: true,
    },
});

const map = {
    folders: Folders,
    languages: Languages,
    search: Search,
    playlist: Playlist,
};
</script>
<template>
    <component :is="map[content] ?? map.folders" />
</template>