/*
    Scripts related to the playlist tab.
    Most of these are click events on the playlist tab.
*/
"use strict";

$(function () {
    var firstLoaded = false;
    var database = [];
    var timeout = 15;

    window.showDatabase = function () {
        return database;
    }

    function renderPlaylist(data) {
        var totalTime = 0;
        $.each(data, function (iterator, songObject) {
            totalTime += songObject.Duration + timeout;
            var position = String(iterator + 1).padStart(2, '0') + ".";
            var errorMeta = songObject.HasError ? "⚠️" : "";
            $("#playlist-songs").append("<a id=\"playlist-songs-" + iterator + "\" href=\"#\" class=\"list-group-item\" data-toggle=\"modal\" data-target=\"#dynamic-modal\">" + errorMeta + position + " " + songObject.Artist + " - " + songObject.Title + " - " + secondsToDate(totalTime) + "<span class=\"glyphicon glyphicon-info-sign\"></span></a>");
            $("#playlist-songs-sortable").append("<a id=\"playlist-songs-sortable-" + iterator + "\" href=\"#\" class=\"list-group-item\" data-toggle=\"modal\" data-target=\"#dynamic-modal\">" + errorMeta + position + " " + songObject.Artist + " - " + songObject.Title + " - " + secondsToDate(totalTime) + "<span class=\"glyphicon glyphicon-info-sign\"></span></a>");
            const songObjectClone = $.extend({}, songObject);
            songObjectClone.Position = iterator;
            songObjectClone.PositionStr = position;
            $("#playlist-songs-" + iterator).data("modal-songObject", JSON.stringify(songObjectClone));
            $("#playlist-songs-sortable-" + iterator).data("modal-songObject", JSON.stringify(songObjectClone));
        });
    }
    
    function showDisconnected() {
        firstLoaded = false;
        if (window.sessionStorage) {
            window.sessionStorage.setItem("preserved-playlist", JSON.stringify(database));
        }
        $("#disconnected").removeClass("hidden");
    }

    function refreshPlaylist() {
        $.get("api/getCurrentPlaylist.json", function (data) {
            if (!data.length && database.length && window.sessionStorage) {
                firstLoaded = false;
                window.sessionStorage.setItem("preserved-playlist", JSON.stringify(database));
            }
            database = data;
            $.get("api/getplaylistTimeout", function (playlistTimeOut) {
                $("#disconnected").addClass("hidden");
                timeout = parseInt(playlistTimeOut);
                clearList("playlist-songs");
                clearList("playlist-songs-sortable");
    
                renderPlaylist(database);

                if (!database.length && !firstLoaded && window.sessionStorage && window.sessionStorage.getItem("preserved-playlist")) {
                    var tempElement = $("<div>");
                    $(tempElement).load("playlist-preserve-modal-body.html", function (data) {
                        $.ajaxSetup({
                            cache: true
                        });
                        localize("#dynamic-modal", true);
                        $("#modal-body").html(data);
                        $("#modal-restore-playlist").on("click", function () {
                            database = JSON.parse(window.sessionStorage.getItem("preserved-playlist"));
                            
                            rebuildPlaylist(0);
                        });
                        $("#modal-remove-playlist").on("click", function () {
                            window.sessionStorage.removeItem("preserved-playlist");
                            refreshPlaylist();
                        });
                        $("#dynamic-modal").modal("show");
                    });
                }
                firstLoaded = true;
            }).fail(showDisconnected);
        }).fail(showDisconnected);
    }

    function rebuildPlaylist(item) {
        var songObject = $.extend({}, database[item]);
        delete songObject.Duration;
        songObject.name = songObject.Artist + " " + songObject.Title;
        $.get("api/getDataBase.json", function () {
            var status = addSong(JSON.stringify(songObject));
            status.done(function () {
                var next = item + 1;
                if (next < database.length) {
                    rebuildPlaylist(next);
                } else {
                    refreshPlaylist();
                    if (window.sessionStorage) {
                        window.sessionStorage.removeItem("preserved-playlist");
                    }
                }
            });
        });
    }

    /*
        On refresh playlist button click make an API call to get the current playlist.
        Once the data is arrived parse it and clear the list and sortable list.
        Then loop through the returned data to append rows to the list.
        Each row is clickable. If clicked it'll show the general information of this song.
        This will be shown through a modal. One can also change the position and remove the song from playlist again.
    */
    $("#refresh-playlist").click(refreshPlaylist);

    $('#reconnect').click(function () {
        if (window.sessionStorage) {
        }
    });
    
    /*
        Whenever an item in the playlist is clicked on a modal will pop up.
        This modal will show general information about the song for example:
            - Artist
            - Song title
            - Language
            - Edition
            - Creator
        The modal also has functionality to move the song up and down one position, set a complete new position or remove the song.
    */
    $("[id^=playlist-songs]").on("click", "a", function () {
        var jsonSongObject = $(this).data("modal-songObject");
        var songObject = JSON.parse(jsonSongObject);
        var title = songObject.PositionStr + " " + songObject.Artist + " - " + songObject.Title;
    
        var tempElement = $("<div>");
        $(tempElement).load("playlist-song-modal-body.html", function (data) {
            window.songObject = songObject;
            $.ajaxSetup({
                cache: true
            });
            $.getScript("js/playlist-song-modal-body.js", function () {
                $("#modal-title").text(title);
                $("#modal-body").html(data);
            });
        });
    });
    
    /*
        Whenever the playlist refresh toggle is changed we will check wether it's set to true or false.
        If true an interval will be globally set to click the refresh playlist button every 10 seconds.
        When set back to false this interval is cleared and the playlist will no longer be updated automaticly.
    */
    $("#refresh-playlist-toggle").change(function () {
        if ($(this).prop("checked")) {
            window.document.userTurnedToggleOff = false;
            window.document.refreshToggleIsOn = true;
            window.IntervalSet = window.setInterval(function () {
                $("#refresh-playlist").click();
            }, 10000);
        } else {
            window.document.refreshToggleIsOn = false;
            window.document.userTurnedToggleOff = true;
            window.clearInterval(window.IntervalSet);
        }
    });
    
    /*
        Whenever the user clicks on the tab playlist it automaticly updates the (maybe) old playlist with the current playlist.
        This is triggered by the refresh playlist button.
    */
    $("a[href='#playlist']").on("shown.bs.tab", function () {
        /*
            Instantiate the Sortable library. This will make the use of drag and drop available.
        */
        $("#preserve-playlist-toggle").bootstrapToggle("on");
        if (!window.document.refreshToggleIsOn && !window.document.userTurnedToggleOff) {
            $("#refresh-playlist-toggle").bootstrapToggle("on");
            window.document.refreshToggleIsOn = true;
        }
        var list = document.getElementById("playlist-songs-sortable");
        Sortable.create(list, {
            group: "words",
            animation: 150,
            scroll: true,
            scrollSensitivity: 150,
            onEnd: function (evt) {
                if (evt.oldIndex === evt.newIndex) {
                    return;
                }
    
                var songId = parseInt(evt.oldIndex);
                var position = parseInt(evt.newIndex);
    
                if(isNaN(songId) || isNaN(position)) {
                    buildAlertMessage("failed_changing_position_of_song!", "danger");
                    return;
                }
    
                var data = {
                    "songId": songId,
                    "position": position
                };
    
                $.ajax({
                    url: "api/setposition",
                    type: "POST",
                    data: JSON.stringify(data),
                    contentType: "application/json; charset=utf-8",
                    success: function(data, textStatus, jqXHR) {
                        buildAlertMessage("successfully_changed_position_of_song.", "success");
                        $("#refresh-playlist").click();
                    },
                    error: function(jqXHR, textStatus, errorThrown) {
                        buildAlertMessage("failed_changing_position_of_song!", "danger");
                    }
                });
            }
        });
        $("#refresh-playlist").click();
    });
});
