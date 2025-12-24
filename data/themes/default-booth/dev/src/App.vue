<script setup lang="ts">
import { computed, provide, watch } from 'vue';
import { Store, useStore } from 'vuex';
import type { State } from './types';
import Header from './Components/Layout/Header.vue';
import Footer from './Components/Layout/Footer.vue';
import Content from './Components/Layout/Content.vue';

const store: Store<State> = useStore();
const content = computed({
    get: () => store.state.screen,
    set(value) {
        const { sort = '', descending = false } = store.state.screenQuery ?? {};
        store.dispatch('setScreenQuery', {
            sort,
            descending,
        });
        store.dispatch('setScreen', value);
    }
});

provide('content', content);

const pageTitle = computed(() => store.state.language.performous_web_frontend ?? 'performous_web_frontend');

function setPageTitle(newPageTitle: string) {
    const title = document.head.querySelector('title');
    if (title) {
        title.textContent = newPageTitle;
    }
}

function switchContent(newContent: string) {
    const { sort = '', descending = false } = store.state.screenQuery ?? {};
    store.dispatch('setScreenQuery', {
        sort,
        descending,
    });
    content.value = newContent;
}

watch(pageTitle, setPageTitle);
setPageTitle(pageTitle.value);
</script>

<template>
  <div class="screen">
    <Header :content @switch="switchContent" />
    <main class="content"><Content :content /></main>
    <Footer />
  </div>
</template>

<style lang="css">
.screen {
    position: fixed;
    display: grid;
    grid-template-columns: 1fr;
    grid-template-rows: max-content 1fr max-content;
    inset: 0;
}

.content {
    display: flex;
    align-items: stretch;
    justify-content: stretch;
    overflow: hidden;
}

.content > * {
    overflow: auto;
    width: 100%;
}
</style>