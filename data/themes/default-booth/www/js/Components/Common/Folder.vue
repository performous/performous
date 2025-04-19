<script setup>
import { computed, defineAsyncComponent } from 'vue';
import { remapFolders } from '@constants';
import Cover from './Cover.vue';
import NoCover from './Cover/NoCover.vue';

const { folder } = defineProps({
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

const asyncComponent = defineAsyncComponent({
    loader: () => import(`./Folders/${remapFolderCover.value}.vue`),
    loadingComponent: NoCover,
    delay: 200,
    errorComponent: null,
    onError: () => null,
});
</script>
<template>
    <component v-if="asyncComponent" :is="asyncComponent" />
    <Cover v-else :file="`folders/${folder}`" :alt="folder" :ignore-missing />
</template>