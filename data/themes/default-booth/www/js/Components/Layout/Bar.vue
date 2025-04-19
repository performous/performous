<script setup>
import Brand from './Bar/Brand.vue';
defineEmits(['switch']);
</script>
<template>
    <div class="top-bar">
        <div class="brand">
            <Brand />
        </div>
        <a href="#" @click.prevent="$emit('switch', 'folders')">List</a>
        <a href="#" @click.prevent="$emit('switch', 'search')">Search</a>
        <a href="#" @click.prevent="$emit('switch', 'playlist')">Queue</a>
        <div class="spacer"></div>
    </div>
</template>