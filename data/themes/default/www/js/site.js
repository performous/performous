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

            clearPlaylist();

            $.each(database, function (iterator, songObject){
                totalTime += songObject.Duration + timeout;
                $("#playlist-songs").append("<a href=\"#\" class=\"list-group-item\" data-toggle=\"modal\" data-target=\"#dynamic-modal\" data-modal-title=\"" + songObject.Artist.replace(/\"/g,'') + " - " + songObject.Title.replace(/\"/g,'') + "\" data-modal-body=\"somebody\">" + songObject.Artist + " - " + songObject.Title + " - " + secondsToDate(totalTime) + "<span class=\"glyphicon glyphicon-info-sign\"></span></a>");
            });
        });
    });
});


function clearPlaylist(){
    var myNode = document.getElementById("playlist-songs");
    while (myNode.firstChild) {
        myNode.removeChild(myNode.firstChild);
    }
}

function secondsToDate(seconds){
    var date = new Date(null);
    date.setSeconds(seconds);
    return date.toISOString().substr(11, 8);
}