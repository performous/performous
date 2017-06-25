"use strict";

$('#refresh-playlist').bootstrapToggle();

$('#refresh-playlist').change(function() {
    console.log("got event");
});

$('#playlist .songs a').click(function(){
    var title = $(this).data('modal-title');
    var body = $(this).data('modal-body');
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