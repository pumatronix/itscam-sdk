<script setup lang="ts">
import DefaultTheme from "vitepress/theme";
import AiSearchSnippets from "./AiSearchSnippets.vue";

const { Layout: DefaultLayout } = DefaultTheme;
</script>

<template>
  <DefaultLayout>
    <template #layout-bottom>
      <AiSearchSnippets />
    </template>
  </DefaultLayout>
</template>
