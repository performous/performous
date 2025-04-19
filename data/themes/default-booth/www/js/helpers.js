const helpers = (() => {
    const exports = {
        getLanguageName(language) {
            let lang = language.trim().replace(/\s*\(romanized\)$/i, '');
            if (constants.remapLanguages[lang]) {
                lang = constants.remapLanguages[lang];
            }

            return lang;
        }
    };

    return exports;
})();