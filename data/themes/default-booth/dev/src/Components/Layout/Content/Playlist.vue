<script setup lang="ts">
import { computed } from 'vue';
import { useStore } from 'vuex';
import SongList from '../../Common/SongList.vue';
import type { Song } from '../../../types';
import duration from '../../../helpers/duration';

const store = useStore();

const list = computed(() => {
    const { song, playlist, timeout } = store.state;

    const baseDuration = (song && typeof song === 'object' ? (song.Duration - song.Position + timeout) : (song ?? 0));

    return playlist.reduce((prev: { [key: string]: any }, song: Song) => {
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
        <div class="heading">Playlist</div>
        <SongList :songs="list" playlist />
    </div>
</template>