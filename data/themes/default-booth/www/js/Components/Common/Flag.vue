<script setup>
import { computed, defineAsyncComponent, ref } from 'vue';
import { remapFlags } from '@constants';
import Cover from './Cover.vue';
import NoCover from './Cover/NoCover.vue';

const { language } = defineProps({
    language: {
        type: String,
        required: true,
    },
    ignoreMissing: {
        type: Boolean,
        required: false,
        default: false,
    },
});

const remapFlagLanguage = computed(() => remapFlags[language] ?? language);
const status = ref('loading');

const asyncComponent = defineAsyncComponent({
    loader: () => {
        status.value = 'loading';
        return import(`./Flags/${remapFlagLanguage.value}.vue`);
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
    <Cover v-else :file="`languages/${language}`" :alt="language" :ignore-missing />
</template>