/*
	Scripts related to the database tab.
	All of these are click events on the database tab.
*/
"use strict";

/*
	On refresh database button click make an API call to get the current loaded songs.
	Once the data is arrived parse it and clear the table.
	Build the table (in memory as one string) and append it to the div which holds all of the songs.
	When the table is build add a songObject as JSON to each row. This is used for adding the song once clicked.
*/
$("#refresh-database").click(function () {
	$.get("api/getDataBase.json", function (data) {
		var database = data;
		clearTable("database-songs");

		var html = buildTable(database);
		$(html).appendTo("#database-songs");

		$.each(database, function (iterator, songObject) {
			$("#database-songs-" + iterator).data("songObject", JSON.stringify(songObject));
		});
	});
});

/*
	One may click on the table headers to sort ascending or descending for that column.
	This will clear the whole table and does a post to the API which sends a sorted list back.
	Build the table (in memory as one string) and append it to the div which holds all of the songs.
	When the table is build add a songObject as JSON to each row. This is used for adding the song once clicked.
*/
$("a[id^='sort-by-']").click(function () {
	var sortOrderToBe = $(this).data("sort-ascending") ? "descending" : "ascending";
	var sortBy = $(this).attr("id").replace("sort-by-", "");
	var url = "api/getDataBase.json?sort=" + sortBy + "&order=" + sortOrderToBe;

	$(this).data("sort-ascending", sortOrderToBe === "descending" ? false : true);
	$(this).find("span").toggleClass("glyphicon-menu-down").toggleClass("glyphicon-menu-up");
	$(this).parent().siblings().children().each(function () {
		$(this).find("span").removeClass("glyphicon-menu-up").addClass("glyphicon-menu-down");
	});

	$.get(url, function (data) {
		var database = data;

		clearTable("database-songs");

		var html = buildTable(database);
		$(html).appendTo("#database-songs");

		$.each(database, function (iterator, songObject) {
			$("#database-songs-" + iterator).data("songObject", JSON.stringify(songObject));
		});
	});
});

/*
	Whenever a row is clicked in the table retrieve the song from the attribute.
	Then it will make a request to add the song to the current playlist.
	If it succeeds the player will be redirected to the playlist screen.
	If it fails the player will notice an alert box saying what did go wrong.
*/
$("#database-songs").on("click", "tr[id^='database-songs-']", function () {
	var songObjectToSend = $(this).data("songObject");
	addSong(songObjectToSend);
});