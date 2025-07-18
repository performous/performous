<script setup lang="ts">
import { computed } from 'vue';
import Cover from './Cover.vue';
import { remapLanguages } from '../../constants';

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

const remapLanguage = computed(() => remapLanguages[language] ?? language);
</script>
<template>
    <Cover :file="`languages/${remapLanguage}`" :alt="language" :ignore-missing />
</template>