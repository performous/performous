<script setup lang="ts">
import SVGLoader from '../Common/SVGLoader.vue';
import Brand from './Header/Brand.vue';
defineEmits(['switch']);
const contentList = {
    folders: 'list',
    search: 'search',
    playlist: 'queue',
};
</script>

<template>
    <header class="header">
        <div class="brand">
            <SVGLoader url="./images/logo.svg">
                <Brand />
            </SVGLoader>
        </div>
        <a
            v-for="(label, key) in contentList"
            :key
            href="#"
            @click.prevent="$emit('switch', key)">
            {{ $translate(label) }}
        </a>
        <div class="spacer"></div>
    </header>
</template>
<style lang="css">
.header {
    padding: 0 1rem;
    display: flex;
    gap: 1rem;
    color: var(--color-primary-fg);
    background-color: var(--color-primary-bg);
}

.header > a {
    padding: 0 1rem;
    display: flex;
    align-items: center;
}

.brand, .header .spacer {
    flex: 1 0 0;
}

.brand {
    padding: 1rem 0;
}

.brand > svg {
    height: 2rem;
}
</style>