<script setup>
import Bar from './Layout/Bar.vue';
import Content from './Layout/Content.vue';
import Footer from './Layout/Footer.vue';

const { computed, ref, watch } = Vue;
const { useStore } = Vuex;

const store = useStore();
const content = computed({
    get: () => store.state.screen,
    set(value) {
        store.dispatch('setScreenQuery', null);
        store.dispatch('setScreen', value);
    }
});

const pageTitle = computed(() => store.state.language.performous_web_frontend ?? 'performous_web_frontend');

function setPageTitle(newPageTitle) {
    document.head.querySelector('title').textContent = newPageTitle;
}

function switchContent(newContent) {
    store.dispatch('setScreenQuery', null);
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