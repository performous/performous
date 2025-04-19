<script setup>
const { computed } = Vue;
const { useStore } = Vuex;

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
    <div class="footer">
        <p v-if="song && typeof song === 'object'">{{ song.Artist }} &mdash; {{ song.Title }} ({{ progress }})</p>
        <p class="credits">{{ $store.state.language.credits }}</p>
    </div>
</template>