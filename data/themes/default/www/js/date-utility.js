/*
    Helper functions for dates.
*/
"use strict";

/*
    Converts the given seconds to a simplified extended ISO format (ISO 8601).
*/
function secondsToDate(seconds) {
    var date = new Date(null);
    date.setSeconds(seconds);
    return date.toISOString().substr(11, 8);
}