/*
    Localization features.
*/
"use strict";

/*
    Take the given selector as parent and find all 'text'.
    For every textnode run it through our localizer.
    This function is used in the 'localize' function.
*/
function localizeInternal(selector, key, value) {
    var textNodes = $(selector).find("*").contents().filter(function () {
        return this.nodeType === Node.TEXT_NODE && this.textContent.trim() === key;
    });

    textNodes.each(function () {
        this.textContent = value;
    });

    $("input[type=text]").each(function () {
        if ($(this).attr("placeholder") === key) {
            $(this).attr("placeholder", value);
        }
    });
}

/*
    Decide wether to use localstorage or retrieve all translations from API.
    Then run 'localizeInternal' to actually translate the found strings.
*/
function localize(selector, fromCache = true) {
    if (fromCache) {
        $.each(localStorage, function (key, value) {
            localizeInternal(selector, key, value);
        });
    } else {
        $.get("api/language", function (data) {
            var localizer = data;
            $.each(localizer, function (key, value) {
                localizeInternal(selector, key, value);
                localStorage.setItem(key, value);
            });
        });
    }
}

/*
    At the start of the application once every HTML is loaded translate the whole body tag.
*/
$(function () {
    localize("head", false);
    localize("body", false);
});