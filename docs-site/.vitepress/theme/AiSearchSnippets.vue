<script setup lang="ts">
import { onMounted, ref } from "vue";

declare const __AI_SEARCH_API_URL__: string;
declare const __AI_SEARCH_SNIPPET_VERSION__: string;

const apiUrl = __AI_SEARCH_API_URL__;
const snippetVersion = __AI_SEARCH_SNIPPET_VERSION__;
const loaded = ref(false);

onMounted(() => {
  if (!apiUrl) return;

  const scriptId = "cf-ai-search-snippet";
  if (document.getElementById(scriptId)) {
    loaded.value = true;
    return;
  }

  const script = document.createElement("script");
  script.id = scriptId;
  script.type = "module";
  script.src = `${apiUrl.replace(/\/$/, "")}/assets/${snippetVersion}/search-snippet.es.js`;
  script.onload = () => {
    loaded.value = true;
  };
  document.head.appendChild(script);
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
      :api-url="apiUrl"
      title="SDK Assistant"
    />
  </div>
</template>
