"use strict";

$.loadScript = function (url, callback) {
    $.ajax({
        url: url,
        dataType: "script",
        success: callback,
        async: true,
        cache: true
    });
};

/*
    Loads the scripts used in the search tab.
    Finishes with the message that all scripts are loaded.
    The website is now fully useable.
*/
function loadSearchJs() {
    $("#custom-scripts-search").load("search.js", function () {
        console.log("All scripts are loaded. The website is now fully useable.");
    });
}

/*
    Loads the scripts used in the playlist tab.
    Continues to loadSearchJs().
*/
function loadPlaylistJs() {
    $("#custom-scripts-playlist").load("playlist.js", loadSearchJs);
}

/*
    Loads the scripts used in the database tab.
    Continues to loadPlaylistJs().
*/
function loadDatabaseJs() {
    $("#custom-scripts-database").load("database.js", loadPlaylistJs);
}

/*
    Loads the helper functions for songs.
    Continues to loadDatabaseJs().
*/
function loadSongsJs() {
    $("#custom-scripts-songs").load("songs.js", loadDatabaseJs);
}

/*
    Loads the helper functions for alerts.
    Continues to loadSongsJs().
*/
function loadAlertJs() {
    $("#custom-scripts-alert").load("alert.js", loadSongsJs);
}

/*
    Loads the helper functions for dates.
    Continues to loadAlertJs().
*/
function loadDateUtilityJs() {
    $("#custom-scripts-date-utility").load("date-utility.js", loadAlertJs);
}

/*
    Loads the helper functions for list items.
    Continues to loadDateUtilityJs().
*/
function loadListItemsJs() {
    $("#custom-script-list-items").load("list-items.js", loadDateUtilityJs);
}

/*
    Loads the helper functions for tables.
    Continues to loadListItemsJs().
*/
function loadTableJs() {
    $.loadScript("table.js", loadListItemsJs);
}

/*
    Fills the localize scripttag.
    This will localize the complete body tag aswell.
    This will always be new translations since we don"t grab them from the cache.
    Continues to loadTableJs().
*/
function loadLocalizeJs() {
    $("#custom-script-localize").load("localize.js", loadTableJs);
}

/*
    Fills the footer div.
    Sends a log to console to say all HTML is loaded.
    Continues to loadLocalizeJs().
*/
function loadFooterHtml() {
    $(".footer").load("footer.html", function () {
        console.log("All HTML is loaded. Let\"s get started on loading scripts.");
        loadLocalizeJs();
    });
}

/*
    Fills the search div.
    Continues to loadFooterHtml().
*/
function loadSearchHtml() {
    $("#search").load("search.html", loadFooterHtml);
}

/*
    Fills the playlist div.
    Continues to loadSearchHtml().
*/
function loadPlaylistHtml() {
    $("#playlist").load("playlist.html", loadSearchHtml);
}

/*
    Fills the database div.
    Continues to loadPlaylistHtml().
*/
function loadDatabaseHtml() {
    $("#database").load("database.html", loadPlaylistHtml);
}

/*
    Fills the custom modal div.
    Continues to loadDatabaseHtml().
*/
function loadGenericModalHtml() {
    $("#custom-modal").load("generic-modal.html", loadDatabaseHtml);
}

/*
    Fills the content div.
    Continues to loadGenericModalHtml().
*/
function loadContentHtml() {
    $(".content").load("content.html", loadGenericModalHtml);
}

/*
    Fills the menu div.
    Continues to loadContentHtml().
*/
function loadMenuHtml() {
    $(".menu").load("menu.html", loadContentHtml);
}

/*
    Fills the header div.
    Continues to loadMenuHtml().
*/
function loadHeaderHtml() {
    $(".header").load("header.html", loadMenuHtml);
}

/*
    Initial call to bootstrap the application once document is ready.
*/
function bootstrapApplication() {
    loadHeaderHtml();
}

/*
    Bootstrapper of the webfrontend.
    This loads each markup file in the desired divs.
*/
$(function () {
    bootstrapApplication();
});