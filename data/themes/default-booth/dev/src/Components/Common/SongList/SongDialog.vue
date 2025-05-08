<script setup lang="ts">
import { computed, ref, useTemplateRef } from 'vue';
import type { Song } from '../../../types';
import { useStore } from 'vuex';
import PlayIcon from '../../Icons/PlayIcon.vue';
import CrossIcon from '../../Icons/CrossIcon.vue';
import ArrowDownIcon from '../../Icons/ArrowDownIcon.vue';
import ArrowUpIcon from '../../Icons/ArrowUpIcon.vue';
import ArrowUpDownIcon from '../../Icons/ArrowUpDownIcon.vue';
import PlusIcon from '../../Icons/PlusIcon.vue';
import Dialog from '../Dialog.vue';

defineProps<{
    songs: Song[]
    playlist: boolean
}>();

const song = defineModel<Song|null>('song');
const index = defineModel<number|null>('index');

const dialogRef = useTemplateRef('dialog');

const dialogElem = computed(() => dialogRef?.value?.dialog);

const desiredIndex = ref<number|null>(null);

const store = useStore();

function closeModal() {
    song.value = null;
    index.value = null;
    if (dialogElem.value) {
        dialogElem.value.close();
    }
}

function remove() {
    store.dispatch('removeSong', { songId: index.value });
    closeModal();
}

function move(position: number) {
    store.dispatch('moveSong', { songId: index.value, position });
    closeModal();
}

function play(songId: number|null = null) {
    store.dispatch('playSong', songId !== null ? { songId: index.value } : song.value);
    closeModal();
}

function enqueue() {
    store.dispatch('addSong', song.value);
    closeModal();
}

function priority() {
    store.dispatch('prioritizeSong', song.value);
    closeModal();
}

defineExpose({
    dialog: dialogElem,
});
</script>

<template>
    <Dialog ref="dialog" class="dialog" @close="closeModal">
        <div v-if="song" class="heading">{{ song.Artist }} &mdash; {{ song.Title }}</div>
        <div class="button-group">
            <template v-if="playlist">
                <button type="button" @click="play(index)">
                    <PlayIcon />
                    <p>{{ $translate('play_song') }}</p>
                </button>
                <button type="button" @click="remove()">
                    <CrossIcon />
                    <p>{{ $translate('remove_song') }}</p>
                </button>
                <button type="button" @click="move(index - 1)" v-if="(index || index === 0) && index > 0">
                    <ArrowUpIcon />
                    <p>{{ $translate('move_up') }}</p>
                </button>
                <button type="button" @click="move(index + 1)" v-if="(index || index === 0) && index < (songs.length - 1)">
                    <ArrowDownIcon />
                    <p>{{ $translate('move_down') }}</p>
                </button>
                <div v-if="songs.length > 1">
                    <button type="button" @click="move(desiredIndex ?? 0)" :disabled="desiredIndex === null">
                        <ArrowUpDownIcon />
                        <p>{{ $translate('set_position') }}:</p>
                    </button>
                    <div class="spacer"></div>
                    <div><input type="number" v-model="desiredIndex" placeholder="Enter position" /></div>
                </div>
            </template>
            <template v-else>
                <button type="button" @click="play()">
                    <PlayIcon />
                    <p>{{ $translate('play_song') }}</p>
                </button>
                <button type="button" @click="enqueue()">
                    <PlusIcon />
                    <p>{{ $translate('add_song') }}</p>
                </button>
                <button type="button" @click="priority()">
                    <ArrowUpIcon />
                    <p>{{ $translate('play_song_next') }}</p>
                </button>
            </template>
        </div>
    </Dialog>
</template>