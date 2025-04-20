<script setup>
import { computed, defineAsyncComponent, ref } from 'vue';
import { remapFolders } from '@constants';
import Cover from './Cover.vue';
import NoCover from './Cover/NoCover.vue';

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
const status = ref('loading');

const asyncComponent = defineAsyncComponent({
    loader: () => {
        status.value = 'loading';
        return import(`./Folders/${remapFolderCover.value}.vue`);
    },
    loadingComponent: NoCover,
    delay: 200,
    errorComponent: null,
    onError: () => status.value = 'error',
    onSuccess: () => status.value = 'success',
});
</script>
<template>
    <component v-if="status !== 'error'" :is="asyncComponent" />
    <Cover v-else :file="`folders/${folder}`" :alt="folder" :ignore-missing />
</template>