<script setup>
import { computed, ref, useTemplateRef } from 'vue';
import { useStore } from 'vuex';
import { useIntersectionObserver } from '@vueuse/core';

import SortIcon from './SongList/SortIcon.vue';
import TriangleAlert from '../Icons/TriangleAlertIcon.vue';
import SongDialog from './SongList/SongDialog.vue';

const { songs } = defineProps({
    songs: {
        type: Array,
        required: false,
        default: () => [],
    },
    playlist: {
        type: Boolean,
        required: false,
        default: false,
    },
});

const renderLimit = defineModel({
    type: Number,
    required: false,
    default: -1,
});

const dialog = ref(null);
const currentSong = ref(null);
const songIndex = ref(null);
const desiredIndex = ref(null);

const store = useStore();

const target = useTemplateRef('target');
const { stop } = useIntersectionObserver(
    target,
    ([entry], observerElement) => {
        if (entry.isIntersecting) {
            renderLimit.value+= 100;
        }
    }
);

const sort = computed({
    get() {
        if (!store.state.screenQuery?.sort) {
            return '';
        }
        return store.state.screenQuery.sort;
    },
    set(value) {
        const newQuery = { ...store.state.screenQuery };
        newQuery.sort = value;
        store.dispatch('setScreenQuery', newQuery);
    },
});

const descending = computed({
    get() {
        if (!store.state.screenQuery?.descending) {
            return false;
        }
        return store.state.screenQuery.descending;
    },
    set(value) {
        const newQuery = { ...store.state.screenQuery };
        newQuery.descending = value;
        store.dispatch('setScreenQuery', newQuery);
    },
});

function setSort(sortType) {
    if (sortType !== sort.value) {
        descending.value = false;
        sort.value = sortType;
    } else if (descending.value) {
        descending.value = false;
        sort.value = '';
    } else {
        descending.value = true;
    }
    const query = [`sort=${sort.value || 'artist'}`, `order=${descending.value ? 'descending' : 'ascending'}`];
    if (store.state.screenQuery.search) {
        query.push(`query=${store.state.screenQuery.search}`);
    }
    store.dispatch('refreshDatabase', query.join('&'));
}

const dialogRef = useTemplateRef('dialog');

function setCurrentSong(song, index) {
    currentSong.value = song;
    songIndex.value = index;
    dialogRef.value.dialog.showModal();
}

</script>
<template>
    <SongDialog ref="dialog" v-model:song="currentSong" v-model:index="songIndex" :songs :playlist />
    <table class="song-list">
        <thead>
            <tr class="bg-primary-bg text-primary-fg leading-4 *:[th]:text-left *:[th]:p-2">
                <th class="artist" @click="setSort('artist')"><SortIcon type="artist" />{{ $translate('artist') }}</th>
                <th class="title" @click="setSort('title')"><SortIcon type="title" />{{ $translate('title') }}</th>
                <th class="creator" @click="setSort('creator')"><SortIcon type="creator" />{{ $translate('creator') }}</th>
                <template v-if="!playlist">
                    <th class="edition" @click="setSort('edition')"><SortIcon type="edition" />{{ $translate('edition') }}</th>
                    <th class="comment">{{ $translate('comment') }}</th>
                </template>
                <th class="year">{{ $translate('year') }}</th>
                <th class="wait-time" v-if="playlist">{{ $translate('wait_time') }}</th>
            </tr>
        </thead>
        <tbody class="*:[tr]:even:bg-secondary-bg *:[tr]:even:text-secondary-fg *:[tr]:cursor-pointer *:[tr]:hover:bg-hover-bg *:[tr]:hover:text-hover-fg **:[td]:text-left **:[td]:p-2">
            <tr v-for="(song, index) in (renderLimit >= 0 ? songs.slice(0, renderLimit) : songs)" @click="setCurrentSong(song, index)" :key="`song-${song.Artist}-${song.Title}-${index}`">
                <td>{{ song.Artist }}</td>
                <td>{{ song.Title }}</td>
                <td>{{ song.Creator }}</td>
                <template v-if="!playlist">
                    <td>{{ song.Edition }}</td>
                    <td>
                        <TriangleAlert v-if="song.HasError" />
                        {{ song.Comment }}</td>
                </template>
                <td>{{ song.Year }}</td>
                <td v-if="playlist" class="right">{{ song.Wait }}</td>
            </tr>
        </tbody>
    </table>
    <div class="text-center p-4" v-if="renderLimit >= 0 && songs.length > renderLimit" ref="target">Loading...</div>
</template>

<style lang="css">
.song-list {
    width: 100%;
    border-collapse: collapse;
}

.song-list > thead > tr {
    color: var(--color-primary-fg);
    background-color: var(--color-primary-bg);
}

.song-list > thead > tr > th {
    text-align: left;
    padding: 0.5rem;
}

.song-list .artist,
.song-list .creator,
.song-list .edition {
    width: 10%;
}

.song-list .year {
    width: 5%;
    min-width: 6rem;
}

.song-list > tbody > tr {
    cursor: pointer;
}

.song-list > tbody > tr:nth-child(2n) {
    color: var(--color-secondary-fg);
    background-color: var(--color-secondary-bg);
}

.song-list > tbody > tr:hover {
    color: var(--color-hover-fg);
    background-color: var(--color-hover-bg);
}

.song-list > tbody > tr > td {
    text-align: left;
    padding: 0.5rem;
}
</style>