<script setup>
import NoCover from './Cover/NoCover.vue';
const { computed, ref, watch } = Vue;
const { useStore } = Vuex;

const { file: fileProp } = defineProps({
    file: {
        type: String,
        required: true,
    },
    alt: {
        type: String,
        required: false,
        default: null,
    }
});

const store = useStore();

const file = computed(() => fileProp);
const image = ref(null);

const loadImage = (newFile) => {
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
    <img v-if="url" :src="url" :alt="alt ?? file" />
    <NoCover v-else />
</template>