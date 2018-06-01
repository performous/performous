/*
    Helper functions for tables.
*/
"use strict";

/*
    Building the database table from a collection of songs.
    This function is used within 'database.js' to generate the table.
    This function outputs one big string containing the complete body of the table.
    This is way faster than adding row by row with JQuery since each add causes a complete redraw.
    We build this up in memory and then draw the complete table one time.
*/
function buildTable(database) {
    var r = [];
    var j = -1;
    for (var key = 0, size = database.length; key < size; key++) {
        r[++j] = "<tr id='database-songs-" + key +"'><td>";
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
        r[++j] = "</td></tr>";
    }
    return r.join("");
}

/*
    Function to clear a table given as selector.
    The table body is completely removed while the headers persist.
    This function is used within 'database.js'.
*/
function clearTable(selector) {
    var tbody = document.getElementById(selector).getElementsByTagName('tbody')[0];
    var tbodyHeader = document.getElementById("table-header");
    var newTbody = document.createElement('tbody');
    newTbody.appendChild(tbodyHeader);
    tbody.parentNode.replaceChild(newTbody, tbody);
}