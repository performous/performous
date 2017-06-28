"use strict";

$('#refresh-playlist-toggle').bootstrapToggle();

$('#refresh-playlist-toggle').change(function() {
    console.log("got event");
});

$('#playlist-songs').on("click", "a", function() {
    console.log("clicked");
    var title = $(this).data('modal-title');
    var body = $(this).data('modal-body');
    console.log(title);
    $('#modal-title').text(title);
    $('#modal-body').text(body);
});


$('#refresh-database').click(function() {
    $.get("api/getDataBase.json", function(data) {
        var database = JSON.parse(data);
        clearTable("#database-songs > tbody");
        $.each(database, function (iterator, songObject){
            var y = "<div>pizza</div>";
            $("#database-songs").append("<tr><td>" + songObject["Artist"] + "</td><td>" + songObject["Title"] + "</td><td class='hidden-xs'>" + songObject["Language"] + "</td><td class='hidden-xs hidden-sm'>" + songObject["Edition"] + "</td><td class='hidden-xs hidden-sm hidden-md'>" + songObject["Creator"] + "</td><td class='text-right text-nowrap fixed-pixel-glyphicon'><span class='glyphicon glyphicon-plus'></span></td></tr>");
        });
    });
});

$('#refresh-playlist').click(function() {
    $.get("api/getCurrentPlaylist.json", function(data) {
        var database = JSON.parse(data);
        $.get("api/getplaylistTimeout", function(data){
            var timeout = parseInt(data);
            var totalTime = 0; 

            clearList("playlist-songs");

            $.each(database, function (iterator, songObject){
                totalTime += songObject.Duration + timeout;
                $("#playlist-songs").append("<a href=\"#\" class=\"list-group-item\" data-toggle=\"modal\" data-target=\"#dynamic-modal\" data-modal-title=\"" + songObject.Artist.replace(/\"/g,'') + " | " + songObject.Title.replace(/\"/g,'') + "\" data-modal-body=\"somebody\">" + songObject.Artist + " - " + songObject.Title + " - " + secondsToDate(totalTime) + "<span class=\"glyphicon glyphicon-info-sign\"></span></a>");
            });
        });
    });
});

$('#search-database').click(function() {
    var query = $('#search-field').val()
    $.post("api/search", query, function(data) {
        var database = JSON.parse(data);

        clearList("searched-songs");

        $.each(database, function (iterator, songObject){
            var songMeta = "";
            songMeta += songObject.Language.length > 0 ? " | " + songObject.Language : "";
            songMeta += songObject.Edition.length > 0 ? " | " + songObject.Edition : "";
            $("#searched-songs").append("<a href=\"#\" id=\"" + iterator + "\" class=\"list-group-item\" >" + songObject.Artist + " - " + songObject.Title + songMeta + "<span class=\"glyphicon glyphicon-plus\"></span></a>");
            $("#"+iterator).data("songObject", JSON.stringify(songObject));
        });
    });
});

$('#searched-songs').on("click", "a", function() {
    var songObjectToSend = $(this).data('songObject');
    console.log(songObjectToSend);
    $.post("api/add", songObjectToSend, function() {
        window.location.href = "#playlist?message=success_added_song&messageType=success"
        $('a[href="#playlist"]').tab('show');
    });
});

$('a[href="#playlist"]').on('shown.bs.tab', function(event){
    $('#refresh-playlist').click();
});


function clearList(selector){
    var myNode = document.getElementById(selector);
    while (myNode.firstChild) {
        myNode.removeChild(myNode.firstChild);
    }
}

function clearTable(selector) {
    var tbody = $(selector);
    while (tbody.children().length > 1) {
        tbody.children().last().remove();
    }
}

function secondsToDate(seconds){
    var date = new Date(null);
    date.setSeconds(seconds);
    return date.toISOString().substr(11, 8);
}