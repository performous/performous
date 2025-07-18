<script setup lang="ts">
import SongList from '../../Common/SongList.vue';
import ChevronLeftIcon from '../../Icons/ChevronLeftIcon.vue';
const { songs } = defineProps({
    songs: {
        type: Array,
        required: false,
        default: () => [],
    },
});

const category = defineModel<string|null>({
    type: String,
    required: false,
    default: null,
});
</script>
<template>
    <div>
        <div class="heading">
            {{ category }}
            <div class="back" @click="category = null"><ChevronLeftIcon /> Back</div>
        </div>
        <SongList :songs />
    </div>
</template>

<style lang="css">
.back {
    display: flex;
    gap: 0.5rem;
    align-items: center;
    font-size: 1rem;
    position: absolute;
    left: 0.5rem;
    top: 0.5rem;
    padding: 1rem;
    color: var(--color-secondary-fg);
    background-color: var(--color-secondary-bg);
    border-radius: 1000rem;
    cursor: pointer;
}

.back > svg {
    width: 1rem;
    height: 1rem;
}
</style>