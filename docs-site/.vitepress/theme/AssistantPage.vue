<script setup lang="ts">
import { onMounted, ref } from "vue";

declare const __AI_SEARCH_API_URL__: string;
declare const __AI_SEARCH_SNIPPET_VERSION__: string;

const apiUrl = __AI_SEARCH_API_URL__;
const snippetVersion = __AI_SEARCH_SNIPPET_VERSION__;
const ready = ref(false);

onMounted(() => {
  if (!apiUrl) return;

  const scriptId = "cf-ai-search-snippet";
  if (document.getElementById(scriptId)) {
    ready.value = true;
    return;
  }

  const script = document.createElement("script");
  script.id = scriptId;
  script.type = "module";
  script.src = `${apiUrl.replace(/\/$/, "")}/assets/${snippetVersion}/search-snippet.es.js`;
  script.onload = () => {
    ready.value = true;
  };
  document.head.appendChild(script);
});
</script>

<template>
  <div class="assistant-page">
    <h1>SDK Assistant</h1>
    <p>
      Ask questions about the ITSCAM SDK or request implementation examples.
      Answers are grounded in the indexed documentation and example sources.
    </p>
    <p v-if="!apiUrl" class="assistant-page__notice">
      The assistant is not configured for this build. Set
      <code>VITE_AI_SEARCH_API_URL</code> to your Cloudflare AI Search public
      endpoint (for example
      <code>https://&lt;instance-id&gt;.search.ai.cloudflare.com/</code>).
    </p>
    <chat-full-page-snippet
      v-else-if="ready"
      :api-url="apiUrl"
      title="ITSCAM SDK Assistant"
    />
    <p v-else class="assistant-page__notice">Loading assistant…</p>
    <p class="assistant-page__warning">
      Do not paste camera passwords or production credentials into the chat.
    </p>
  </div>
</template>

<style scoped>
.assistant-page {
  max-width: 960px;
  margin: 0 auto;
  padding: 1rem 0 3rem;
}

.assistant-page__notice {
  color: var(--vp-c-text-2);
  font-size: 0.95rem;
}

.assistant-page__warning {
  margin-top: 1.5rem;
  color: var(--vp-c-text-2);
  font-size: 0.875rem;
}
</style>
