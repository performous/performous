<script setup lang="ts">
import { computed } from 'vue';
import { useStore } from 'vuex';
import ChevronDownIcon from '../../Icons/ChevronDownIcon.vue';
import ChevronUpIcon from '../../Icons/ChevronUpIcon.vue';
defineProps({
    type: {
        type: String,
        required: true,
    }
});

const store = useStore();

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
        <div v-else></div>
    </div>
</template>

<style lang="css">
.sort-icon {
    width: 1em;
    margin-right: 0.5rem;
    display: inline-block;
}

.sort-icon > * {
    width: 1em;
    height: 1em;
}
</style>