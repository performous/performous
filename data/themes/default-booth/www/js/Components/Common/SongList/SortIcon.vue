<script setup>
import { computed } from 'vue';
import ChevronDownIcon from '../../Icons/ChevronDownIcon.vue';
import ChevronUpIcon from '../../Icons/ChevronUpIcon.vue';
defineProps({
    type: {
        type: String,
        required: true,
    }
});

const isPlaylist = computed(() => store.state.screen === 'playlist');

const sort = computed(() => {
    if (!store.state.screenQuery?.sort) {
        return '';
    }
    return store.state.screenQuery.sort;
});

const descending = computed(() => {
    if (!store.state.screenQuery?.descending) {
        return false;
    }
    return store.state.screenQuery.descending;
});

</script>
<template>
    <div class="sort-icon" v-if="!isPlaylist">
        <template v-if="sort === type">
            <ChevronDownIcon v-if="descending" />
            <ChevronUpIcon v-else />
        </template>
    </div>
</template>