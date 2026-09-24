/*
    Scripts related to the search tab.
    Most of these are click events on the search tab.
*/
"use strict";

/*
    Whenever click upon the search tab we will load the current database.
    Once successfully retrieved this will be the source of the autocompleter.
    The autocomplete function shows 10 items and doesn"t select items per default.
    If an item from the autocomplete is clicked "addSong" will be called to add the clicked song.
    After this the search field will be cleared and if the result is successful the player will be redirected to the playlist tab.
    If the API call failed an alert will show up.
    The autocomplete makes use of the TypeAhead library.
*/
$("#search-tab").click(function (){
    $.get("api/getDataBase.json?sort=artist&order=ascending", function (data) {
        var database = data;
        var input = $("#search-field");
        input.typeahead({
            source: database,
            autoSelect: false,
            items: 10,
            select: function (e) {
                var val = this.$menu.find(".active").data("value");
                if(val) {
                    addSong(JSON.stringify(val));
                    $("#search-field").val("");
                } else {
                    $("#search-database").click();
                    $(".typeahead.dropdown-menu").hide();
                }
            }
        });
    });
});

/*
    When the search button is pressed an API call will be made with the given query.
    This will result into a list of song which match the criteria.
    First the current list will be cleared up for the new search results.
    Then the results will be appended to the list.
    If there are no matches found an alert will pop up telling so.
*/
$("#search-database").click(function (e, callback) {
    var query = $("#search-field").val();
    if(query.length == 0) {
        $.get("api/getDataBase.json?sort=artist&order=ascending", function (data) {
            var database = data;
            clearList("searched-songs");

            $.each(database, function (iterator, songObject) {
                var songMeta = "";
                songMeta += songObject.Language.length > 0 ? " | " + songObject.Language : "";
                songMeta += songObject.Edition.length > 0 ? " | " + songObject.Edition : "";
                songMeta += songObject.ProvidedBy.length > 0 ? " | " + songObject.ProvidedBy : "";
                songMeta += songObject.Comment.length > 0 ? " | " + songObject.Comment : "";
                var errorMeta = songObject.HasError ? "⚠️" : "";
                $("#searched-songs").append("<a href=\"#\" id=\"searched-songs-" + iterator + "\" class=\"list-group-item\" >" + errorMeta + songObject.Artist + " - " + songObject.Title + songMeta + "<span class=\"glyphicon glyphicon-plus\"></span></a>");
                $("#searched-songs-" + iterator).data("songObject", JSON.stringify(songObject));
            });

            if(database.length === 0) {
                buildAlertMessage("no_songs_found_with_current_filtery.", "warning");
            }
            if(typeof callback === "function") {
                callback();
            }
        });
        return;
    }
    var searchData = {
        "query": query,
    };

    $.ajax({
        url: "api/search",
        type: "POST",
        data: JSON.stringify(searchData),
        contentType: "application/json; charset=utf-8",
        success: function(data, textStatus, jqXHR) {
            var database = data;

            clearList("searched-songs");
            $.each(database, function (iterator, songObject) {
                var songMeta = "";
                songMeta += songObject.Language.length > 0 ? " | " + songObject.Language : "";
                songMeta += songObject.Edition.length > 0 ? " | " + songObject.Edition : "";
                songMeta += songObject.Source.length > 0 ? " | " + songObject.Source : "";
                songMeta += songObject.App.length > 0 ? " | " + songObject.App : "";
                songMeta += songObject.AppVersion.length > 0 ? " | " + songObject.AppVersion : "";
                songMeta += songObject.Comment.length > 0 ? " | " + songObject.Comment : "";
                var errorMeta = songObject.HasError ? "⚠️" : "";
                $("#searched-songs").append("<a href=\"#\" id=\"searched-songs-" + iterator + "\" class=\"list-group-item\" >" + errorMeta + songObject.Artist + " - " + songObject.Title + songMeta + "<span class=\"glyphicon glyphicon-plus\"></span></a>");
                $("#searched-songs-" + iterator).data("songObject", JSON.stringify(songObject));
            });

            if(database.length === 0) {
                buildAlertMessage("no_songs_found_with_current_filter.", "warning");
            }
            if(typeof callback === "function") {
                callback();
            }
        },
        error: function(jqXHR, textStatus, errorThrown) {
            
        }
    });
});

/*
    Whenever the return key (enter key) is pressed within the search field an API call is made.
    This will retrieve the matching search results.
    If no results are found an alert will be shown telling so.
*/
$("#search-field").keypress(function (e) {
    if (e.which === 13) {
        $("#search-database").click();
    }
});

/*
    Whenever a row is clicked in the list retrieve the song from the attribute.
    Then it will make a request to add the song to the current playlist.
    If it succeeds the player will be redirected to the playlist screen.
    If it fails the player will notice an alert box saying what did go wrong.
*/
$("#searched-songs").on("click", "a", function () {
    var songObjectToSend = $(this).data("songObject");
    addSong(songObjectToSend);
});