import { defineConfig } from 'astro/config';
import tailwind from "@astrojs/tailwind";
import solidJs from "@astrojs/solid-js";
import compress from "astro-compress";

// https://astro.build/config
export default defineConfig({
  integrations: [tailwind(), solidJs(), compress({ html: { collapseBooleanAttributes: true, removeComments: true, removeEmptyAttributes: true, removeRedundantAttributes: true } })],
  vite: {
    ssr: {
      external: ["svgo"],
    },
  },
});