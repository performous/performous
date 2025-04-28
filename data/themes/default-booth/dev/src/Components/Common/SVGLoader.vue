<script setup lang="ts">
import { computed, h, ref, type VNode, watch } from 'vue';
import type { Data } from '../../types';

const { url: urlProp = null } = defineProps<{
    url: string|null,
}>();

const url = computed(() => urlProp);

const isLoading = ref<boolean>(false);
const isLoaded = ref<boolean>(false);
const svgComponent = ref<VNode|null>(null);

function parseSVG(nodes:HTMLCollection):VNode[] {
    const components:VNode[] = [];

    for (let index = 0; index < nodes.length; index += 1) {
        const node:Element = nodes[index];
        
        const name = node.nodeName;
        const attributes:Data = {};
        for (const attribute of node.attributes) {
            attributes[attribute.name] = attribute.value;
        }
        const children:VNode[] = node.children.length ? parseSVG(node.children) : [];

        components.push(h(name, attributes, children));
    }
    return components;
}

async function loadSVG(file:string|null) {
    isLoading.value = true;
    isLoaded.value = false;
    if (!file) {
        isLoading.value = false;
        svgComponent.value = null;
        return;
    }
    try {
        const res = await fetch(file);
        if (!res.ok) {
            throw new Error();
        }
        const svg = await res.text();

        const parser = new DOMParser();
        const doc = parser.parseFromString(svg, 'image/svg+xml');

        svgComponent.value = parseSVG(doc.children)[0];
        isLoading.value = false;
        isLoaded.value = true;
    } catch (_e) {
        isLoading.value = false;
        svgComponent.value = null;
    }
}

watch(url, loadSVG);
loadSVG(url.value);
</script>
<template>
    <slot name="placeholder" v-if="$slots.placeholder && (isLoading || (!svgComponent && !$slots.default))" />
    <component v-else-if="svgComponent" :is="svgComponent" />
    <slot v-else />
</template>
