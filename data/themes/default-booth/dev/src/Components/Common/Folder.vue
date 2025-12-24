<script setup lang="ts">
import { computed } from 'vue';
import Cover from './Cover.vue';
import { remapFolders } from '../../constants';

const { folder, ignoreMissing } = defineProps({
    folder: {
        type: String,
        required: true,
    },
    ignoreMissing: {
        type: Boolean,
        required: false,
        default: false,
    },
});

const remapFolderCover = computed(() => remapFolders[folder] ?? folder);
</script>
<template>
    <Cover :file="`folders/${remapFolderCover}`" :alt="folder" :ignore-missing />
</template>