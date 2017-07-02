var update = false;
var invertedsort = false;
var database = {};
var AutocompleteList = {};
var songlist = {};
var playlistScreenTimeout = 0;
var localizer = {};



<!-- Home -->

$("#databaseRefreshButton").bind("click", function(event) {
    $.get("api/getDataBase.json", function(data) {
        database = JSON.parse(data);

        var s = $('input[name=radio-choice-Sort]:checked', '#radioButtonForm').val();
        switch (s) {
            case '1':
                database = sortByKey(database, 'Title');
                database = sortByKey(database, 'Artist');
                break;
            case '2':
                database = sortByKey(database, 'Artist');
                database = sortByKey(database, 'Title');
                break;
            case '3':
                database = sortByKey(database, 'Artist');
                database = sortByKey(database, 'Language');
                break;
            case '4':
                database = sortByKey(database, 'Artist');
                database = sortByKey(database, 'Edition');
                break;
            case '5':
                database = sortByKey(database, 'Artist');
                database = sortByKey(database, 'Creator');
                break;
        }
        var table = document.getElementById("songtable");
        for (var i = table.rows.length - 1; i > 0; i--) {
            table.deleteRow(i);
        }
        if (invertedsort) {
            for (var i = database.length - 1; i >= 0; i--) {
                $("#songtable").last().append("<tr><td>" + database[i].Artist + "</td><td>" + database[i].Title + "</td><td>" + database[i].Language + "</td><td>" + database[i].Edition + "</td><td>" + database[i].Creator + "</td> </tr>");
            }
        } else {
            for (i in database) {
                $("#songtable").last().append("<tr><td>" + database[i].Artist + "</td><td>" + database[i].Title + "</td><td>" + database[i].Language + "</td><td>" + database[i].Edition + "</td><td>" + database[i].Creator + "</td> </tr>");
            }
        }
    });
});

function sortByKey(array, key) {
    return array.sort(function(a, b) {
        var x = a[key];
        var y = b[key];
        return ((x < y) ? -1 : ((x > y) ? 1 : 0));
    });
}

function AddSong(number, autocomplete) {
    if (autocomplete) {
        $.post("api/add", JSON.stringify(AutocompleteList[number]));
        $('#searchv2').val("added: " + AutocompleteList[number].Title);
    } else {
        $.post("api/add", JSON.stringify(database[number]));
        $('#searchv2').val("added: " + database[number].Title);
    }

    RefreshPlayList();

}

<!-- Playlist -->

$("#playlistRefreshButton").bind("click", function(event) {
    RefreshPlayList();
});

function RefreshPlayList() {
    $.get("api/getCurrentPlaylist.json", function(data) {
        songlist = JSON.parse(data);
        if (playlistScreenTimeout == 0) {
            var timeout = "";
            jQuery.ajaxSetup({
                async: false
            });
            $.get("api/getplaylistTimeout", function(data) {
                playlistScreenTimeout = parseInt(data);

            });
            jQuery.ajaxSetup({
                async: true
            });
        }
        $('#playListView').html('<li data-role="list-divider" role="heading">Upcoming songs:</li><li data-theme="c">\n');
        var totalduration = 0;
        for (i in songlist) {
            var count = parseInt(i, 10) + parseInt(1, 10);
            var hours = 0;
            var minutes = 0;
            var sec = totalduration;
            while (sec >= 60) {
                minutes++;
                sec -= 60;
            }
            while (minutes >= 60) {
                hours++;
                minutes -= 60;
            }

            var durationtext;
            if (hours > 0) {
                durationtext = ("00" + hours).slice(-2) + ':' + ("00" + minutes).slice(-2) + ':' + ("0" + sec).slice(-2);
            } else {
                durationtext = ("00" + minutes).slice(-2) + ':' + ("0" + sec).slice(-2);
            }
            $('#playListView').append('<li data-theme="c" data-icon="info"><a href="javascript:ViewInfo(' + i + ', false)" data-transition="slide"> #' + count + ' ' + CreateSongString(songlist[i]) + ' +' + durationtext + '</a></li>\n');
            totalduration += songlist[i].Duration + playlistScreenTimeout;
        }
        $('#playListView').listview('refresh');
    });
}

$("#databaseSearchButton").bind("click", function(event) {
    $("#autocomplete").hide();
    DoQuery();
});

$(document).keypress(function(e) {
    if (e.which == 13) {
        $("#autocomplete").hide();
        DoQuery();
    }
});

function CreateSongString(song, showEdition) {
    $totalString = song.Artist + ' - ' + song.Title;
    if (showEdition) {
        if (song.Language && song.Edition) {
            $totalString += ' | ' + song.Language + ' - ' + song.Edition;
        } else if (song.Language && !song.Edition) {
            $totalString += ' | ' + song.Language;
        } else if (!song.Language && song.Edition) {
            $totalString += ' | ' + song.Edition;
        }
    } else {
        if (song.Language) {
            $totalString += ' | ' + song.Language;
        }
    }

    return $totalString;
}

function DoQuery() {
    $("#autocomplete").prev().find("input").attr("id", "searchv2");
    var query = $('#searchv2').val();

    results = $.post("api/search", query, function(data) {
        database = JSON.parse(data);
        $('#searchResultsViewer').html('<li data-role="list-divider" role="heading">Available songs:</li> <li data-theme="c">\n');
        for (i in database) {

            $('#searchResultsViewer').append('<li data-theme="c" data-icon="plus"><a href="javascript:AddSong(' + i + ')" data-transition="slide">' + CreateSongString(database[i], true) + '</a></li>');
        }
        $('#searchResultsViewer').listview('refresh');
    });
}

function ViewInfo(songnumber) {
	moveup = songnumber - 1;
	movedown = songnumber + 1;
    $('#songinfo').html("Title: " + songlist[songnumber].Title + "<br>\nArtist: " + songlist[songnumber].Artist +
        "<br>\nEdition: " + songlist[songnumber].Edition + "<br>\nLanguage: " + songlist[songnumber].Language + "<br>\nCreator: " +
        songlist[songnumber].Creator + "<br>\n" + " <div class=\"ui-grid-a\">" +
        "<div class=\"ui-block-a\">" +
        "<a id=\"btnremove\" data-role=\"button\" href=\"javascript:removeSong(" + songnumber + ")\" data-icon=\"delete\">" +
        "Remove song </a> </div>" +
        "<div class=\"ui-block-b\"> <a id=\"btncancel\" data-role=\"button\" href=\"#page2\" data-icon=\"back\">" +
        "Cancel </a> </div> </div> <br> " +
        "<div class=\"ui-grid-a\">" +
        "<div class=\"ui-block-a\">" +
        "<a id=\"btnmoveup\" data-role=\"button\" href=\"javascript:setPosition(" + songnumber + ", " + moveup + ")\" data-icon=\"arrow-u\">" +
        "Move up</a></div><div class=\"ui-block-b\"> " +
        "<a id=\"btnmovedown\" data-role=\"button\" href=\"javascript:setPosition(" + songnumber + ", " + movedown + ")\" data-icon=\"arrow-d\">" +
        "Move Down </a> </div> </div> <br>" +
        "<div class=\"ui-grid-a\">" +
        "<div class=\"ui-block-a\">" +
        "<input id=\"position\" placeholder=\"Add a position\" type=\"number\" min=\"1\" max=\"" + songlist.length + "\" data-clear-btn=\"true\" pattern=\"[0-9]*\" /></div><div class=\"ui-block-b\"> " +
        "<a id=\"btnposition\" data-role=\"button\" href=\"javascript:setPosition(" + songnumber + ")\" data-icon=\"recycle\">" +
        "Set Position </a> </div> </div>"
    );
    <!--refresh the buttons so they are rendered correctly-->
    $('#btnremove').button();
    $('#btnmoveup').button();
    $('#btnmovedown').button();
    $('#btncancel').button();
    $('#btnposition').button();
    $('#position').textinput();
    $.mobile.changePage("#page4", {
        role: "dialog"
    });

}

function removeSong(songnumber) {
    $.post("api/remove", songnumber.toString());
    RefreshPlayList();
    $.mobile.changePage("#page2");
}

function setPosition(songnumber, pos) {			
	if(pos || pos === 0) {
		$position = pos;
	} else {
        $position = Math.floor($('#position').val() - 1);
	}
    var data = {
        "songId": songnumber,
        "position": $position
    };
    $.post("api/setposition", JSON.stringify(data));
    RefreshPlayList();
    $.mobile.changePage("#page2");
}

$('#flipupdate').bind("change", function(event, ui) {
    update = !update;
});

$('#flipsortmode').bind("change", function(event, ui) {
    invertedsort = !invertedsort;
});

$("#autocomplete").on("listviewbeforefilter", function(e, data) {
    var $ul = $(this),
        $input = $(data.input),
        value = $input.val(),
        html = "";
    $ul.html("");
    if (value && value.length > 2) {
        $("#autocomplete").show();
        results = $.post("api/autocomplete", $input.val(), function(data) {
            AutocompleteList = JSON.parse(data);
            $ul.html("<li><div class='ui-loader'><span class='ui-icon ui-icon-loading'></span></div></li>");
            for (i in AutocompleteList) {
                var count = parseInt(i, 10) + parseInt(1, 10);
                html += '<li data-theme="c" data-icon="info"><a href="javascript:AddSong(' + i + ',true)" data-transition="slide">' + ' ' + AutocompleteList[i].Artist + ' - ' + AutocompleteList[i].Title + '</a></li>\n';
            }
            $ul.html(html);
            $ul.listview("refresh");
            $ul.trigger("updatelayout");
        });
    }
});

$("#autocomplete").on("click", "li", function() {
    $("#autocomplete").hide();
    DoQuery();
});
window.setInterval(function() {
    if (update) RefreshPlayList();
}, 10000);

$(function () {
    $.get("api/language", function(data) {
        localizer = JSON.parse(data);
        $.each( localizer, function( key, value ) { 

            // this is quite an expensive method....  
            var $textNodes = $("*").contents().filter(function() {
                return this.nodeType === Node.TEXT_NODE && this.textContent.match(key);
            });

            $textNodes.each(function() {                
                 this.textContent = value;                                    
            });
        });
    });
})