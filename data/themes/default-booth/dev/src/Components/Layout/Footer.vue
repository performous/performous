<script setup lang="ts">
import { computed } from 'vue';
import { useStore } from 'vuex';
import duration from '../../helpers/duration';

const store = useStore();

const song = computed(() => store.state.song);

const progress = computed(() => {
    if (typeof song.value !== 'object') {
        return '';
    }
    const length = Math.floor(song.value.Duration);
    const position = Math.floor(song.value.Position);
    const withHour = length >= 3600;

    return `${duration(position, withHour)} / ${duration(length, withHour)}`;
});
</script>

<template>
    <footer class="footer">
        <p v-if="song && typeof song === 'object'" class="pb-4">{{ song.Artist }} &mdash; {{ song.Title }} ({{ progress }})</p>
        <p class="credits">{{ $translate('credits') }}</p>
        <div v-if="song && typeof song === 'object'" class="progress">
            <div class="bar" :style="{
                width: `${(song.Position / song.Duration) * 100}%`
            }"></div>
        </div>
    </footer>
</template>

<style lang="css">
.footer {
    padding: 1rem;
    text-align: center;
    color: var(--color-primary-fg);
    background-color: var(--color-primary-bg);
}

.credits {
    font-size: 0.75rem;
}

.progress {
    position: absolute;
    left: 0;
    bottom: 0;
    width: 100%;
    height: 0.25rem;
}

.progress > .bar {
    height: 100%;
    background-color: var(--color-primary-fg);
}
</style>