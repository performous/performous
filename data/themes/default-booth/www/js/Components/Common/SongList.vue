<script setup>
import { computed, onMounted, ref } from 'vue';
import { useStore } from 'vuex';

import SortIcon from './SongList/SortIcon.vue';
import ArrowDown from '../Icons/ArrowDownIcon.vue';
import ArrowUp from '../Icons/ArrowUpIcon.vue';
import ArrowUpDown from '../Icons/ArrowUpDownIcon.vue';
import Cross from '../Icons/CrossIcon.vue';
import Play from '../Icons/PlayIcon.vue';
import Plus from '../Icons/PlusIcon.vue';
import TriangleAlert from '../Icons/TriangleAlertIcon.vue';

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
})

const dialog = ref(null);
const currentSong = ref(null);
const songIndex = ref(null);
const desiredIndex = ref(null);

const store = useStore();

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

function setCurrentSong(song, index) {
    currentSong.value = song;
    songIndex.value = index;
    dialog.value.showModal();
}

function closeModal() {
    currentSong.value = null;
    songIndex.value = null;
    dialog.value.close();
}

function remove() {
    store.dispatch('removeSong', { songId: songIndex.value });
    closeModal();
}

function move(position) {
    store.dispatch('moveSong', { songId: songIndex.value, position });
    closeModal();
}

function play(songId = null) {
    store.dispatch('playSong', songId !== null ? { songId: songIndex.value } : currentSong.value);
    closeModal();
}

function enqueue() {
    store.dispatch('addSong', currentSong.value);
    closeModal();
}

function priority() {
    store.dispatch('prioritizeSong', currentSong.value);
    closeModal();
}

onMounted(() => {
    dialog.value.addEventListener('click', closeModal);
})
</script>
<template>
    <dialog ref="dialog">
        <div class="dialog-content" @click.stop>
            <div v-if="currentSong" class="header">{{ currentSong.Artist }} &mdash; {{ currentSong.Title }}</div>
            <div v-if="playlist" class="buttons">
                <button class="action" @click="play(songIndex)">
                    <Play />
                    <p>Play song</p>
                </button>
                <button class="action" @click="remove()">
                    <Cross />
                    <p>Remove from queue</p>
                </button>
                <button class="action" @click="move(songIndex - 1)" v-if="songIndex > 0">
                    <ArrowUp />
                    <p>Move song up</p>
                </button>
                <button class="action" @click="move(songIndex + 1)" v-if="songIndex < (songs.length - 1)">
                    <ArrowDown />
                    <p>Move song down</p>
                </button>
                <div v-if="songs.length > 1">
                    <button class="action" @click="move(desiredIndex)" :disabled="desiredIndex === null">
                        <ArrowUpDown />
                        <p>Move song to:</p>
                    </button>
                    <div><input type="number" v-model="desiredIndex" placeholder="Enter position" /></div>
                </div>
            </div>
            <div v-else class="buttons">
                <button class="action" @click="play()">
                    <Play />
                    <p>Play song</p>
                </button>
                <button class="action" @click="enqueue()">
                    <Plus />
                    <p>Add to queue</p>
                </button>
                <button class="action" @click="priority()">
                    <ArrowUp />
                    <p>Play next</p>
                </button>
            </div>
        </div>
    </dialog>
    <table class="songlist">
        <thead>
            <tr>
                <th class="artist" @click="setSort('artist')"><SortIcon type="artist" />{{ $store.state.language.artist }}</th>
                <th class="title" @click="setSort('title')"><SortIcon type="title" />{{ $store.state.language.title }}</th>
                <th class="creator" @click="setSort('creator')"><SortIcon type="creator" />{{ $store.state.language.creator }}</th>
                <template v-if="!playlist">
                    <th class="edition" @click="setSort('edition')"><SortIcon type="edition" />{{ $store.state.language.edition }}</th>
                    <th class="comment">Comment</th>
                </template>
                <th class="year">Year</th>
                <th class="wait" v-if="playlist">Wait time</th>
            </tr>
        </thead>
        <tbody>
            <tr v-for="(song, index) in songs" @click="setCurrentSong(song, index)" :key="`song-${song.Artist}-${song.Title}-${index}`">
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
</template>