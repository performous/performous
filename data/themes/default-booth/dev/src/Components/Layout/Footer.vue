<script setup lang="ts">
import { computed, useTemplateRef } from 'vue';
import { useStore } from 'vuex';
import duration from '../../helpers/duration';
import FastForwardIcon from '../Icons/FastForwardIcon.vue';
import PreviousIcon from '../Icons/PreviousIcon.vue';
import Dialog from '../Common/Dialog.vue';
import RefreshIcon from '../Icons/RefreshIcon.vue';

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

function reconnect() {
    store.dispatch('refreshDatabase', store.state.screen === 'search' && store.state.screenQuery?.search ? store.state.screenQuery.search : '');
}

function restorePlaylist() {
    store.dispatch('restorePlaylist');
}

function removePlaylist() {
    store.dispatch('removePlaylist');
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
        <div v-if="store.state.offline" class="offline">
            {{ $translate('performous_has_disconnected.') }}
            
            <div class="button-group">
                <button type="button" @click="reconnect">
                    <RefreshIcon />
                    <div>{{ $translate('reconnect') }}</div>
                </button>
            </div>
        </div>
        <div v-else-if="store.state.hasPreservedPlaylist" class="preserved-playlist">
            {{ $translate('a_previous_playlist_has_been_found.') }}
            <div class="button-group">
                <button type="button" @click="restorePlaylist">
                    {{ $translate('restore_playlist') }}
                </button>
                <button type="button" @click="removePlaylist">
                    {{ $translate('remove_playlist') }}
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

.current-playing, .offline, .preserved-playlist {
    position: relative;
    margin-bottom: 1rem;
}

.current-playing .button-group,
.offline .button-group {
    position: absolute;
    right: 0;
    top: 0;
    display: flex;
    gap: 1rem;
}

.current-playing .button-group button,
.offline .button-group button {
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
    font-size: 0.625rem;
}

.preserved-playlist .button-group {
    margin-top: 1rem;
    display: flex;
    gap: 1rem;
    align-items: center;
    justify-content: center;
}

.preserved-playlist .button-group button {
    background-color: var(--color-secondary-bg);
    color: var(--color-secondary-fg);
    padding: 1rem;
}

.preserved-playlist .button-group button:hover {
    background-color: var(--color-hover-bg);
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