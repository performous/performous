<script setup>
import SongList from '../../../Common/SongList.vue';
const { songs } = defineProps({
    songs: {
        type: Array,
        required: true,
    },
});

const category = defineModel({
    type: String,
    required: true,
});
</script>
<template>
    <div>
        <div class="header">
            {{ category }}
            <div class="back" @click="category = null">&lt; Back</div>
        </div>
        <SongList :songs />
    </div>
</template>