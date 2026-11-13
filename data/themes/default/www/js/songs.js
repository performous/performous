/*
    Helper functions for songs.
*/
"use strict";

/*
    Adds a song to the current playlist.
    This function is used within 'search.js' and 'database.js'.
    After the API call it builds an alert wether it succeeded or failed.
    Upon success the player will be moved to the playlist tab.
*/
function addSong(songObjectToSend, silent = false) {
    return $.ajax({
        url: "api/add",
        type: "POST",
        data: songObjectToSend,
        contentType: "application/json; charset=utf-8",
        success: function(data, textStatus, jqXHR) {
            if (!silent) {
                buildAlertMessage("successfully_added_song_to_the_playlist.", "success");
            }
            $("a[href='#playlist']").tab("show");
        },
        error: function(jqXHR, textStatus, errorThrown) {
            buildAlertMessage("failed_adding_song_to_the_playlist!", "danger");
        }
    });
}