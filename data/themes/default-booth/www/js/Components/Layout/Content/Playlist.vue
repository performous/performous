<script setup>
import { computed } from 'vue';
import { useStore } from 'vuex';
import SongList from '../../Common/SongList.vue';

const store = useStore();

const list = computed(() => {
    const { song, playlist, timeout } = store.state;

    const baseDuration = (song && typeof song === 'object' ? (song.Duration - song.Position + timeout) : (song ?? 0));

    return playlist.reduce((prev, song) => {
        prev.list.push({
            ...song,
            Wait: duration(prev.duration, true),
        });
        prev.duration+= song.Duration + timeout;
        return prev;
    }, {list: [], duration: baseDuration}).list;
});
</script>
<template>
    <div>
        <SongList :songs="list" playlist />
    </div>
</template>