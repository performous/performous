<script setup lang="ts">
import { onMounted, ref } from 'vue';
import type { Song } from '../../../types';
import { useStore } from 'vuex';
import PlayIcon from '../../Icons/PlayIcon.vue';
import CrossIcon from '../../Icons/CrossIcon.vue';
import ArrowDownIcon from '../../Icons/ArrowDownIcon.vue';
import ArrowUpIcon from '../../Icons/ArrowUpIcon.vue';
import ArrowUpDownIcon from '../../Icons/ArrowUpDownIcon.vue';
import PlusIcon from '../../Icons/PlusIcon.vue';

defineProps<{
    songs: Song[]
    playlist: boolean
}>();

const song = defineModel<Song|null>('song');
const index = defineModel<number|null>('index');

const dialog = ref<HTMLDialogElement|null>(null);
const desiredIndex = ref<number|null>(null);

const store = useStore();

function closeModal() {
    song.value = null;
    index.value = null;
    if (dialog.value) {
        dialog.value.close();
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
    dialog,
});

onMounted(() => {
    if (dialog.value) {
        dialog.value.addEventListener('click', closeModal);
    }
});
</script>

<template>
    <dialog ref="dialog" class="dialog">
        <div class="content" @click.stop>
            <div v-if="song" class="heading">{{ song.Artist }} &mdash; {{ song.Title }}</div>
            <div class="button-group">
                <template v-if="playlist">
                    <button type="button" @click="play(index)">
                        <PlayIcon />
                        <p>Play song</p>
                    </button>
                    <button type="button" @click="remove()">
                        <CrossIcon />
                        <p>Remove from queue</p>
                    </button>
                    <button type="button" @click="move(index - 1)" v-if="(index || index === 0) && index > 0">
                        <ArrowUpIcon />
                        <p>Move song up</p>
                    </button>
                    <button type="button" @click="move(index + 1)" v-if="(index || index === 0) && index < (songs.length - 1)">
                        <ArrowDownIcon />
                        <p>Move song down</p>
                    </button>
                    <div v-if="songs.length > 1">
                        <button type="button" @click="move(desiredIndex ?? 0)" :disabled="desiredIndex === null">
                            <ArrowUpDownIcon />
                            <p>Move song to:</p>
                        </button>
                        <div class="spacer"></div>
                        <div><input type="number" v-model="desiredIndex" placeholder="Enter position" /></div>
                    </div>
                </template>
                <template v-else>
                    <button type="button" @click="play()">
                        <PlayIcon />
                        <p>Play song</p>
                    </button>
                    <button type="button" @click="console.log('x'); enqueue()">
                        <PlusIcon />
                        <p>Add to queue</p>
                    </button>
                    <button type="button" @click="priority()">
                        <ArrowUpIcon />
                        <p>Play next</p>
                    </button>
                </template>
            </div>
        </div>
    </dialog>
</template>

<style lang="css">
.dialog {
    position: fixed;
    overflow: hidden;
    max-width: 100%;
    max-height: 100%;
    width: fit-content;
    margin: auto;
    color: var(--color-secondary-fg);
    background-color: var(--color-secondary-bg);
}

.dialog > .content {
    display: block;
    padding: 1rem;
    width: 100%;
    height: 100%;
    overflow: auto;
}

.dialog .heading {
    font-size: 1.5rem;
    width: 100%;
    overflow: hidden;
    white-space: nowrap;
    text-overflow: ellipsis;
    padding: 1rem;
}

.dialog .button-group {
    display: flex;
    gap: 1rem;
    justify-content: center;
    width: 100%;
}

.dialog .button-group > * {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    color: var(--color-primary-fg);
    background-color: var(--color-primary-bg);
}

.dialog .button-group > button {
    padding: 1rem;
    cursor: pointer;
}

.dialog .button-group > div {
    justify-content: stretch;
}

.dialog .button-group > div > button {
    display: flex;
    flex-direction: column;
    align-items: center;
    width: 100%;
    cursor: pointer;
    padding: 1rem;
}

.dialog .button-group > div input {
    max-width: 100%;
    padding: 0.5rem 1rem;
    color: var(--color-primary-fg);
    background-color: var(--color-primary-bg);
}

.dialog .button-group > div input::placeholder {
    color: var(--color-primary-fg);
}

.dialog .button-group > div .spacer {
    width: 100%;
    padding: 0 1rem;
}

.dialog .button-group > div .spacer::before {
    content: '';
    display: block;
    border-top: 2px solid var(--color-primary-fg);
}
</style>
