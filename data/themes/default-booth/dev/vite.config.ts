import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

const headPlugin = () => {
  return {
    name: 'no-attribute',
    transformIndexHtml(html: string) {
      const tags: string[] = [
        '  <link rel="stylesheet" href="/css/style.css" />',
      ];
      return html.replace(/<\/head>/, `${tags.join('')}${'\n'}  </head>`);
    },
  };
}

// https://vite.dev/config/
export default ({ command } : { command:string }) => {
  // Object.assign(process.env, loadEnv(mode, process.cwd()));
  
  return defineConfig({
    plugins: [
      vue(),
      headPlugin(),
    ],
    publicDir: (command === 'serve' ? 'public' : false),
    build: {
      outDir: '../www',
      rollupOptions: {
        output: [
          {
            entryFileNames: 'js/[name].js',
            assetFileNames: 'css/[name].[ext]',
          },
        ],
      },
    },
  });
};
