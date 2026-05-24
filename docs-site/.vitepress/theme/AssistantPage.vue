<script setup lang="ts">
import { useAiSearchScript } from "./useAiSearchScript";

const { apiUrl, ready } = useAiSearchScript();
</script>

<template>
  <div class="assistant-page">
    <p class="assistant-page__lead">
      Converse com a documentação do ITSCAM SDK — clients binary, REST e CGI,
      wrappers, tutoriais, exemplos e <code>AGENTS.md</code>. As respostas são
      ancoradas no corpus indexado via Cloudflare AI Search.
    </p>

    <p v-if="!apiUrl" class="assistant-page__notice">
      O assistant não está configurado neste build. Defina
      <code>VITE_AI_SEARCH_API_URL</code> com o public endpoint da instância
      (por exemplo
      <code>https://&lt;instance-id&gt;.search.ai.cloudflare.com/</code>).
    </p>

    <div
      v-else-if="ready"
      class="assistant-page__chat"
      aria-label="ITSCAM SDK Assistant"
    >
      <chat-page-snippet
        :api-url="apiUrl"
        placeholder="Pergunte sobre o SDK ou peça um exemplo de implementação…"
      />
    </div>

    <p v-else class="assistant-page__notice">Carregando assistant…</p>

    <p class="assistant-page__warning">
      Não cole senhas de câmera nem credenciais de produção no chat.
    </p>
  </div>
</template>

<style scoped>
.assistant-page {
  max-width: none;
  margin: 0;
  padding: 0 0 2rem;
}

.assistant-page__lead {
  max-width: 52rem;
  margin: 0 0 1.25rem;
  color: var(--vp-c-text-2);
  font-size: 1rem;
  line-height: 1.6;
}

.assistant-page__chat {
  display: block;
  width: 100%;
  min-height: min(720px, calc(100vh - var(--vp-nav-height, 64px) - 11rem));
  border: 1px solid var(--vp-c-divider);
  border-radius: 12px;
  overflow: hidden;
  background: var(--vp-c-bg-soft);
}

.assistant-page__chat chat-page-snippet {
  display: block;
  width: 100%;
  height: min(720px, calc(100vh - var(--vp-nav-height, 64px) - 11rem));
}

.assistant-page__notice {
  color: var(--vp-c-text-2);
  font-size: 0.95rem;
}

.assistant-page__warning {
  margin-top: 1rem;
  color: var(--vp-c-text-3);
  font-size: 0.875rem;
}
</style>
