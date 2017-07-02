"use strict"
$(function (){
	var songObject = window.songObject;
	window.songObject = null;

	localize();

	if(songObject.Language.length > 1) {
		$("#playlist-song-modal-body #modal-language-div").show();
		$("#playlist-song-modal-body #modal-language-value").text(songObject.Language);
	} else {
		$("#playlist-song-modal-body #modal-language-div").hide();
	}

	if(songObject.Edition.length > 1) {
		$("#playlist-song-modal-body #modal-edition-div").show();
		$("#playlist-song-modal-body #modal-edition-value").text(songObject.Edition);
	} else {
		$("#playlist-song-modal-body #modal-edition-div").hide();
	}

	if(songObject.Creator.length > 1) {
		$("#playlist-song-modal-body #modal-creator-div").show();
		$("#playlist-song-modal-body #modal-creator-value").text(songObject.Creator);
	} else {
		$("#playlist-song-modal-body #modal-creator-div").hide();
	}

	$('#playlist-song-modal-body #modal-move-song-up').click(function() {
	    moveSongUp(songObject.Position)
	});

	$('#playlist-song-modal-body #modal-move-song-down').click(function() {
	    moveSongDown(songObject.Position)
	});

	$('#playlist-song-modal-body #modal-set-position-btn').click(function() {
	    setSongToPosition(songObject.Position)
	});

	$('#playlist-song-modal-body #modal-remove-song').click(function() {
	    removeSongFromPlaylist(songObject.Position)
	});

	function moveSongUp(songId) {
		setSongToPosition(songId, songId - 1, "move_up_song");
	}

	function moveSongDown(songId) {
		setSongToPosition(songId, songId + 1, "move_down_song");
	}

	function setSongToPosition(songId, newPosition, message = "changed_position_song") {
		if(newPosition || newPosition === 0) {
			var position = newPosition;
		} else {
	        var position = Math.floor($('#modal-set-position-input').val() - 1);
		}

	    var data = {
	        "songId": songId,
	        "position": position
	    };

	    $.post("api/setposition", JSON.stringify(data), function() {
	    	window.location.href = "#playlist?message=succes_"+message+"&messageType=success"
        	$('#refresh-playlist').click();
	    });
	}

	function removeSongFromPlaylist(songId) {
		$.post("api/remove", songId.toString(), function() {
        	window.location.href = "#playlist?message=success_removed_song&messageType=success"
        	$('#refresh-playlist').click();
    	});
	}
});