<script setup lang="ts">
import { computed, useTemplateRef } from 'vue';
import { useStore } from 'vuex';
import duration from '../../helpers/duration';
import FastForwardIcon from '../Icons/FastForwardIcon.vue';
import PreviousIcon from '../Icons/PreviousIcon.vue';
import Dialog from '../Common/Dialog.vue';

const store = useStore();

const dialogRef = useTemplateRef('skip');

const song = computed(() => store.state.song);

const progress = computed(() => {
    if (typeof song.value !== 'object') {
        return '';
    }
    const length = Math.floor(song.value.Duration);
    const position = Math.floor(song.value.Position);
    const withHour = length >= 3600;

    const positionTime = duration(position, withHour);
    const lengthTime = duration(length, withHour);

    return `${positionTime} / ${lengthTime}`;
});

function restart() {
    store.dispatch('playSong', song.value);
}

function skipConfirm() {
    dialogRef.value?.dialog?.showModal();
}

function skip() {
    store.dispatch('skipSong');
    closeSkipConfirm();
}

function closeSkipConfirm() {
    dialogRef.value?.dialog?.close();
}
</script>

<template>
    <Dialog ref="skip" class="skip-confirm" @close="closeSkipConfirm">
        <div class="heading">{{ $translate('skip') }}?</div>
        
        <div class="button-group">
            <button type="button" @click="skip">{{ $translate('yes') }}</button>
            <button type="button" @click="closeSkipConfirm">{{ $translate('no') }}</button>
        </div>
    </Dialog>
    <footer class="footer">
        <div
            v-if="song && typeof song === 'object'"
            class="current-playing"
        >
            {{ song.Artist }} &mdash; {{ song.Title }} ({{ progress }})
            <div class="button-group">
                <button type="button" @click="restart">
                    <PreviousIcon />
                    <div>{{ $translate('restart') }}</div>
                </button>
                <button type="button" @click="skipConfirm">
                    <FastForwardIcon />
                    <div>{{ $translate('skip') }}</div>
                </button>
            </div>
        </div>
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

.current-playing {
    position: relative;
    margin-bottom: 1rem;
}

.current-playing .button-group {
    position: absolute;
    right: 0;
    top: 0;
    display: flex;
    gap: 1rem;
}

.current-playing .button-group button {
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
    font-size: 0.625rem;
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