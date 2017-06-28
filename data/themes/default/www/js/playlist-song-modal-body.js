"use strict"
$(function (){
	var songObject = window.songObject;
	window.songObject = null;

	$("#playlist-song-modal-body #modal-language-value").text(songObject.Language);
	$("#playlist-song-modal-body #modal-edition-value").text(songObject.Edition);
	$("#playlist-song-modal-body #modal-creator-value").text(songObject.Creator);
});