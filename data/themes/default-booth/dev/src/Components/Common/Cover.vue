<script setup lang="ts">
import { computed, ref, watch } from 'vue';
import NoCover from './Cover/NoCover.vue';
import SVGLoader from './SVGLoader.vue';

const { file: fileProp } = defineProps({
    file: {
        type: String,
        required: true,
    },
    alt: {
        type: String,
        required: false,
        default: null,
    },
    ignoreMissing: {
        type: Boolean,
        required: false,
        default: false,
    },
});

const file = computed(() => fileProp);
const image = ref<string|null>(null);

const loadImage = (newFile: string) => {
    image.value = null;

    const img = new Image();
    img.src = `./images/${newFile}.jpg`;
    img.addEventListener('load', () => {
        image.value = newFile;
    });
}

const url = computed(() => image.value ? `./images/${image.value}.jpg` : null);

watch(file, loadImage);
loadImage(file.value);
</script>
<template>
    <SVGLoader :url="`./images/${file}.svg`">
        <template #placeholder>
            <div class="cover-placeholder"></div>
        </template>

        <template #default>
            <img v-if="url" :src="url" :alt="alt ?? file" />
            <NoCover v-else-if="!ignoreMissing" />
        </template>
    </SVGLoader>
</template>

<style lang="css">
.cover-placeholder {
    aspect-ratio: 1/1;
    width: 100%;
}
</style>