<script setup>
import { inject } from 'vue';

const content = inject('content');

const buttons = {
    "folders": "Playlists",
    "languages": "Languages"
};
</script>
<template>
    <div class="content-switcher">
        <button v-for="(label, cat) in buttons" type="button" @click="content = cat" :disabled="content === cat">{{ label }}</button>
    </div>
</template>