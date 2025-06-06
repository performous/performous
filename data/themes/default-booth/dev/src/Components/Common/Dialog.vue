<script setup lang="ts">
import { onMounted, ref } from 'vue';

const dialogElem = ref<HTMLDialogElement|null>(null);

const emit = defineEmits(['close']);

defineExpose({
    dialog: dialogElem,
});

function closeModal() {
    if (dialogElem.value) {
        dialogElem.value.close();
    }
    emit('close');
}

onMounted(() => {
    if (dialogElem.value) {
        dialogElem.value.addEventListener('click', closeModal);
    }
});
</script>
<template>
    <dialog ref="dialogElem" class="dialog">
        <div class="content" @click.stop>
            <slot />
        </div>
    </dialog>
</template>


<style lang="css">
.dialog {
    position: fixed;
    overflow: hidden;
    max-width: 100%;
    max-height: 100%;
    width: fit-content;
    margin: auto;
    color: var(--color-secondary-fg);
    background-color: var(--color-secondary-bg);
}

.dialog > .content {
    display: block;
    padding: 1rem;
    width: 100%;
    height: 100%;
    overflow: auto;
}

.dialog .heading {
    font-size: 1.5rem;
    width: 100%;
    overflow: hidden;
    white-space: nowrap;
    text-overflow: ellipsis;
    padding: 1rem;
}

.dialog .button-group {
    display: flex;
    gap: 1rem;
    justify-content: center;
    width: 100%;
}

.dialog .button-group > * {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    color: var(--color-primary-fg);
    background-color: var(--color-primary-bg);
}

.dialog .button-group > button {
    padding: 1rem;
    cursor: pointer;
}

.dialog .button-group > div {
    justify-content: stretch;
}

.dialog .button-group > div > button {
    display: flex;
    flex-direction: column;
    align-items: center;
    width: 100%;
    cursor: pointer;
    padding: 1rem;
}

.dialog .button-group > div input {
    max-width: 100%;
    padding: 0.5rem 1rem;
    color: var(--color-primary-fg);
    background-color: var(--color-primary-bg);
}

.dialog .button-group > div input::placeholder {
    color: var(--color-primary-fg);
}

.dialog .button-group > div .spacer {
    width: 100%;
    padding: 0 1rem;
}

.dialog .button-group > div .spacer::before {
    content: '';
    display: block;
    border-top: 2px solid var(--color-primary-fg);
}
</style>
