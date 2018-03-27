/*
    Javascript to localize and add functionality to our playlist modal.
*/
"use strict";
$(function () {
    /*
        Moves the given song to a desired position.
        After the api call it builds a alert message to show wether the operation succeeded or not.
        Under the hood this function uses 'buildAlertMessage' functionality to reduce copied code.
        The function 'buildAlertMessage' can be found within 'site.js'.
    */
    function setSongToPosition(songId, newPosition, successMessage = "successfully_changed_position_of_song", failureMessage = "failed_changing_position_of_song") {
        var position = 0;
        if (newPosition || newPosition === 0) {
            position = newPosition;
        } else {
            position = Math.floor($("#modal-set-position-input").val() - 1);
        }

        var data = {
            "songId": songId,
            "position": position
        };

        $.post("api/setposition", JSON.stringify(data), function () {
            buildAlertMessage(successMessage, "success");
            $("#refresh-playlist").click();
        }).fail(function () {
            buildAlertMessage(failureMessage, "danger");
        });
    }

    /*
        Moves the given song one position up.
        Under the hood this function uses "setSongToPosition" functionality to reduce copied code.
    */
    function moveSongUp(songId) {
        setSongToPosition(songId, songId - 1, "successfully_moved_song_up", "failed_moving_song_up");
    }

    /*
        Moves the given song one position down.
        Under the hood this function uses "setSongToPosition" functionality to reduce copied code.
    */
    function moveSongDown(songId) {
        setSongToPosition(songId, songId + 1, "successfully_moved_song_down", "failed_moving_song_down");
    }

    /*
        Removes the given song from the playlist.
        After the api call it builds a alert message to show wether the operation succeeded or not.
        Under the hood this function uses 'buildAlertMessage' functionality to reduce copied code.
        The function 'buildAlertMessage' can be found within 'site.js'.
    */
    function removeSongFromPlaylist(songId) {
        $.post("api/remove", songId.toString(), function () {
            buildAlertMessage("successfully_removed_song_from_playlist", "success");
            $("#refresh-playlist").click();
        }).fail(function () {
            buildAlertMessage("failed_removing_song_from_playlist", "danger");
        });
    }

    /*
        Initialiation of our clicked song.
    */
    var songObject = window.songObject;
    window.songObject = null;


    /*
        Localize our complete modal.
        Under the hood this uses the 'localize' function listed in 'site.js'.
    */
    localize("#dynamic-modal", true);

    /*
        If our song contains a language show it.
        Else hide the complete div.
    */
    if (songObject.Language.length > 1) {
        $("#playlist-song-modal-body #modal-language-div").show();
        $("#playlist-song-modal-body #modal-language-value").text(songObject.Language);
    } else {
        $("#playlist-song-modal-body #modal-language-div").hide();
    }

    /*
        If our song contains an edition show it.
        Else hide the complete div.
    */
    if (songObject.Edition.length > 1) {
        $("#playlist-song-modal-body #modal-edition-div").show();
        $("#playlist-song-modal-body #modal-edition-value").text(songObject.Edition);
    } else {
        $("#playlist-song-modal-body #modal-edition-div").hide();
    }

    /*
        If our song contains a creator show it.
        Else hide the complete div.
    */
    if (songObject.Creator.length > 1) {
        $("#playlist-song-modal-body #modal-creator-div").show();
        $("#playlist-song-modal-body #modal-creator-value").text(songObject.Creator);
    } else {
        $("#playlist-song-modal-body #modal-creator-div").hide();
    }

    /*
        Click event handlers to either change a position of the song or remove the song.
    */
    $("#playlist-song-modal-body #modal-move-song-up").click(function () {
        moveSongUp(songObject.Position);
    });

    $("#playlist-song-modal-body #modal-move-song-down").click(function () {
        moveSongDown(songObject.Position);
    });

    $("#playlist-song-modal-body #modal-set-position-btn").click(function () {
        setSongToPosition(songObject.Position);
    });

    $("#playlist-song-modal-body #modal-remove-song").click(function () {
        removeSongFromPlaylist(songObject.Position);
    });
});