<script setup>
import { computed, provide, watch } from 'vue';
import { useStore } from 'vuex';
import Bar from './Layout/Bar.vue';
import Content from './Layout/Content.vue';
import Footer from './Layout/Footer.vue';

const store = useStore();
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

function setPageTitle(newPageTitle) {
    document.head.querySelector('title').textContent = newPageTitle;
}

function switchContent(newContent) {
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
    <div class="app">
        <Bar :content @switch="switchContent" />
        <div class="content"><Content :content /></div>
        <Footer />
    </div>
</template>