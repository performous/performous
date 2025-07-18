# Performous web frontend - Booth

A new web frontend for Performous that emulates the karaoke booth experience.

## Compilation

To compile the frontend, you'll be using `npm`.

```
npm install
npm run build
```

This will automatically compile the application into the `www` folder. This
will overwrite the `index.html`, `css/index.css` and `js/index.js` files.

## Development

During development, you can run the development version.

```
npm run dev
```

To change the ports used, create a new `.env.local` file and use the variables
set in `.env`. **When making changes to the main Performous repo, do not modify
the `.env` file itself.** The `.env.local` file will not be included in the repo,
however, the `.env` file will be.

There might be a few issues running the development version when trying to
interface with the API backend. You can therefore also build in watch mode.

```
npm run build:watch
```

Note that this will overwrite the aforementioned files.

## Other assets

When running a development build, you can use the `public` folder to put assets
you need to test the application. These are mostly the folders and languages cover art,
the logo, the favicon and the `css/style.css` file.