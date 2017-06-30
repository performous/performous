"use strict";

$('#refresh-playlist-toggle').bootstrapToggle('off');
var list = document.getElementById("playlist-songs");
Sortable.create(list, {
    group: "words",
    animation: 150,
    onEnd: function(evt){
        if(evt.oldIndex == evt.newIndex) {
            return;
        }

        var data = {
            "songId": evt.oldIndex,
            "position": evt.newIndex
        };

        $.post("api/setposition", JSON.stringify(data), function() {
            window.location.href = "#playlist?message=succes_changed_position_song&messageType=success"
            $('#refresh-playlist').click();
        });
    }
});

$('#refresh-playlist-toggle').change(function() {
    if($(this).prop('checked')){
        window.IntervalSet = window.setInterval(function () {
            $("#refresh-playlist").click();
        }, 10000);
    } else {
        window.clearInterval(window.IntervalSet);
    }
});

$('#playlist-songs').on("click", "a", function() {
    var jsonSongObject = $(this).data("modal-songObject");
    var songObject = JSON.parse(jsonSongObject);
    var title = songObject.Artist + " - " + songObject.Title;

    var tempElement = $("<div>");
    $(tempElement).load("playlist-song-modal-body.html", function(data){
        window.songObject = songObject;
        $.ajaxSetup({
          cache: true
        });
        $.getScript("js/playlist-song-modal-body.js",  function() {
            $('#modal-title').text(title);
            $('#modal-body').html(data);
        });        
    });    
});


$('#refresh-database').click(function() {
    $.get("api/getDataBase.json", function(data) {
        var database = JSON.parse(data);
        clearTable("#database-songs > tbody");
        var rows = "";
        $.each(database, function (iterator, songObject){
            var y = "<div>pizza</div>";
            var row = "<tr id=\"database-songs-" + iterator + "\"><td>" + songObject.Artist + "</td><td>" + songObject.Title + "</td><td class='hidden-xs'>" + songObject.Language + "</td><td class='hidden-xs hidden-sm'>" + songObject.Edition + "</td><td class='hidden-xs hidden-sm hidden-md'>" + songObject.Creator + "</td><td class='text-right text-nowrap fixed-pixel-glyphicon'><span class='glyphicon glyphicon-plus'></span></td></tr>";
            //$("#database-songs").append("<tr id=\"database-songs-" + iterator + "\"><td>" + songObject.Artist + "</td><td>" + songObject.Title + "</td><td class='hidden-xs'>" + songObject.Language + "</td><td class='hidden-xs hidden-sm'>" + songObject.Edition + "</td><td class='hidden-xs hidden-sm hidden-md'>" + songObject.Creator + "</td><td class='text-right text-nowrap fixed-pixel-glyphicon'><span class='glyphicon glyphicon-plus'></span></td></tr>");
            //$("#database-songs-"+iterator).data("songObject", JSON.stringify(songObject)); //CAREEEEEEEEEEEEEEEEEEEEEEEEEEE
            rows = rows +row;
        });
        $(rows).appendTo("#database-songs");
    });
});

$('#addonce').on('click', function(){
    rows='';
    for (i=0; i<5000; i++)
    {
        var row='<tr><td>test</td></tr>';
        rows=rows+row;
    }
    $(rows).appendTo('table');
});

$("a[id^='sort-by-']").click(function() {
    var sortOrderToBe = $(this).data('sort-ascending') ? "descending" : "ascending";
    var sortBy = $(this).attr('id').replace('sort-by-', '');
    var url = "api/getDataBase.json?sort=" + sortBy + "&order=" + sortOrderToBe;

    $(this).data('sort-ascending', sortOrderToBe === "descending" ? false : true);
    $(this).find('span').toggleClass('glyphicon-menu-down').toggleClass('glyphicon-menu-up');
    $(this).parent().siblings().children().each(function (){
        $(this).find('span').removeClass("glyphicon-menu-up").addClass("glyphicon-menu-down");
    });
    $.get(url, function(data) {
        var database = JSON.parse(data);
        clearTable("#database-songs > tbody");
        $.each(database, function (iterator, songObject){
            var y = "<div>pizza</div>";
            $("#database-songs").append("<tr id=\"database-songs-" + iterator + "\"><td>" + songObject.Artist + "</td><td>" + songObject.Title + "</td><td class='hidden-xs'>" + songObject.Language + "</td><td class='hidden-xs hidden-sm'>" + songObject.Edition + "</td><td class='hidden-xs hidden-sm hidden-md'>" + songObject.Creator + "</td><td class='text-right text-nowrap fixed-pixel-glyphicon'><span class='glyphicon glyphicon-plus'></span></td></tr>");
            $("#database-songs-"+iterator).data("songObject", JSON.stringify(songObject));
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
                $("#playlist-songs").append("<a id=\"playlist-songs-" + iterator + "\" href=\"#\" class=\"list-group-item\" data-toggle=\"modal\" data-target=\"#dynamic-modal\">" + songObject.Artist + " - " + songObject.Title + " - " + secondsToDate(totalTime) + "<span class=\"glyphicon glyphicon-info-sign\"></span></a>");
                songObject.Position = iterator;
                $("#playlist-songs-"+iterator).data("modal-songObject", JSON.stringify(songObject));
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
            $("#searched-songs").append("<a href=\"#\" id=\"searched-songs-" + iterator + "\" class=\"list-group-item\" >" + songObject.Artist + " - " + songObject.Title + songMeta + "<span class=\"glyphicon glyphicon-plus\"></span></a>");            
            $("#searched-songs-"+iterator).data("songObject", JSON.stringify(songObject));
        });
    });
});

$('#searched-songs').on("click", "a", function() {
    var songObjectToSend = $(this).data('songObject');
    addSong(songObjectToSend)
});

$('#database-songs').on("click", "tr[id^='database-songs-']", function() {
    var songObjectToSend = $(this).data('songObject');
    addSong(songObjectToSend)
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

function addSong(songObjectToSend) {
    $.post("api/add", songObjectToSend, function() {
        window.location.href = "#playlist?message=success_added_song&messageType=success"
        $('a[href="#playlist"]').tab('show');
    });
}