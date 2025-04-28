<script setup lang="ts">
import { inject } from 'vue';

const content = inject('content');

const buttons = {
    "folders": "Playlists",
    "languages": "Languages"
};
</script>
<template>
    <div class="content-bar">
        <button v-for="(label, cat) in buttons" type="button" @click="content = cat" :disabled="content === cat">{{ label }}</button>
    </div>
</template>

<style lang="css">
.content-bar {
    display: flex;
    gap: 1rem;
    justify-content: center;
    padding: 1rem 0;
}
.content-bar > button {
    font-size: 1.25rem;
    padding: 0.5rem 1rem;
    border-radius: 1000rem;
    color: var(--color-general-fg);
    cursor: pointer;
}

.content-bar > button:disabled {
    color: var(--color-primary-fg);
    background-color: var(--color-primary-bg);
}
</style>