<script setup lang="ts">
import { computed } from "vue";
import { useRoute } from "vitepress";
import { useAiSearchScript } from "./useAiSearchScript";

const route = useRoute();
const { apiUrl, ready: loaded } = useAiSearchScript();

/** Full-page chat lives on /assistant; skip the floating bubble there. */
const showChatBubble = computed(() => {
  const path = route.path.replace(/\/$/, "") || "/";
  return path !== "/assistant";
});
</script>

<template>
  <div v-if="apiUrl && loaded" class="itscam-ai-search">
    <search-modal-snippet
      :api-url="apiUrl"
      placeholder="Search ITSCAM SDK docs…"
      max-results="10"
    />
    <chat-bubble-snippet
      v-if="showChatBubble"
      :api-url="apiUrl"
      title="SDK Assistant"
    />
  </div>
</template>
