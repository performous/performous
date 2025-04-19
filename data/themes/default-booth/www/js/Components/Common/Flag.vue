<script setup>
import { computed, defineAsyncComponent } from 'vue';
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

const asyncComponent = defineAsyncComponent({
    loader: () => import(`./Flags/${remapFlagLanguage.value}.vue`),
    loadingComponent: NoCover,
    delay: 200,
    errorComponent: null,
    onError: () => null,
});
</script>
<template>
    <component v-if="asyncComponent" :is="asyncComponent" />
    <Cover v-else :file="`languages/${language}`" :alt="language" :ignore-missing />
</template>