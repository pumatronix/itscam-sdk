import DefaultTheme from "vitepress/theme";
import Layout from "./Layout.vue";
import MetadataVisualizer from "./MetadataVisualizer.vue";
import "./custom.css";

export default {
  extends: DefaultTheme,
  Layout,
  enhanceApp(ctx) {
    DefaultTheme.enhanceApp?.(ctx);
    ctx.app.component("MetadataVisualizer", MetadataVisualizer);
  },
};
