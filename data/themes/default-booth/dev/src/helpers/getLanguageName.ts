import { remapLanguages } from "../constants";

export default function getLanguageName(language: string): string {
    let realLanguage: string = language.trim().replace(/\s?\(romanized\)$/i, '');
    if (remapLanguages[realLanguage]) {
        realLanguage = remapLanguages[realLanguage];
    }
    return realLanguage;
}