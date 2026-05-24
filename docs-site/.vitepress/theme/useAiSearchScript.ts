import { onMounted, ref } from "vue";

declare const __AI_SEARCH_API_URL__: string;
declare const __AI_SEARCH_SNIPPET_VERSION__: string;

/** Load the Cloudflare AI Search snippet bundle once per page session. */
export function useAiSearchScript() {
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

  return { apiUrl, ready };
}
