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
            buildAlertMessage("successfully_changed_position_of_song", "success");
            $('#refresh-playlist').click();
        }).fail(function() {
            buildAlertMessage("failed_changing_position_of_song", "danger");
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

        var html = buildTable(database);
        $(html).appendTo("#database-songs");

        $.each(database, function (iterator, songObject){
           $("#database-songs-"+iterator).data("songObject", JSON.stringify(songObject));
        });
    });
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

        var html = buildTable(database);
        $(html).appendTo("#database-songs");
        
        $.each(database, function (iterator, songObject){
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

$('#search-database').click(function(e, callback) {
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

        if(database.length == 0) {
            buildAlertMessage("no_songs_found_with_current_filter", "warning")
        }
        if(typeof callback == "function") {
            callback();
        }
    });
});

$("#search-tab").click(function (){
    $.get("api/getDataBase.json", function(data) {
        var database = JSON.parse(data);
        var input = $("#search-field");
        input.typeahead({
            source: database,
            autoSelect: true,
            items: 10,
            updater: function(item) {
                input.val(item.name);
                $('#search-database').trigger("click", function(){
                    $("#searched-songs").children().first().click();
                }); 
            }
        });
    });
});

$('#search-field').keypress(function (e) {
    if (e.which == 13) {
        $('#search-database').click();
    }
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

$(function () {
    localize("body", false);
});

function localize(selector, fromCache = true) {
    if(fromCache){
        $.each( localStorage, function( key, value ) { 
            localizeInternal(selector, key, value);
        });
    } else {
        $.get("api/language", function(data) {
            var localizer = JSON.parse(data);
            $.each( localizer, function( key, value ) { 
                localizeInternal(selector, key, value);
                localStorage.setItem(key,value);
            });
        });
    }
    
};

function localizeInternal(selector, key, value) {
    var textNodes = $(selector).find("*").contents().filter(function() {
        return this.nodeType === Node.TEXT_NODE && this.textContent.trim() == key;
    });

    textNodes.each(function() {                
         this.textContent = value;                                    
    });

    $('input[type=text]').each(function()
    {   
        if($(this).attr('placeholder') == key) { 
            $(this).attr("placeholder", value);
        }
    });
}


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
        buildAlertMessage("successfully_added_song_to_the_playlist", "success");
        $('a[href="#playlist"]').tab('show');
    }).fail(function() {
        buildAlertMessage("failed_adding_song_to_the_playlist", "danger")
    });
}

function buildTable(database){
    var r = new Array();
    var j = -1;
    for (var key=0, size=database.length; key<size; key++){
        r[++j] ="<tr id='database-songs-" + key +"'><td>";
        r[++j] = database[key].Artist;
        r[++j] = "</td><td>";
        r[++j] = database[key].Title;
        r[++j] = "</td><td class='hidden-xs'>";
        r[++j] = database[key].Language;
        r[++j] = "</td><td class='hidden-xs hidden-sm'>";
        r[++j] = database[key].Edition;
        r[++j] = "</td><td class='hidden-xs hidden-sm hidden-md'>";
        r[++j] = database[key].Creator;
        r[++j] = "</td><td class='text-right text-nowrap fixed-pixel-glyphicon'>";
        r[++j] = "<span class='glyphicon glyphicon-plus'></span>";
        r[++j] = "</td></tr>"
    }

    return r.join("");
}

function buildAlertMessage(message, messageType){
    var localizedMessage = localStorage.getItem(message);
    var innerhtml = "<div class=\"container-fluid\"><div class=\"alert alert-"+messageType+" alert-dismissable\"><a href=\"#\" class=\"close\" data-dismiss=\"alert\" aria-label=\"close\">&times;</a>"+localizedMessage+"</div></div>";
    $("#alert-messages").append(innerhtml);
    window.setTimeout(function() {
        $(".alert").fadeTo(500, 0).slideUp(500, function(){
            $(this).remove(); 
        });
    }, 3000);
}

