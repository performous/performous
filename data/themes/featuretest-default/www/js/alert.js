/*
	Helper functions for translated alerts.
*/
"use strict";

/*
	Function which builts an alert message and gives it a specific type.
	All bootstrap alert types are available:
		- alert-success
		- alert-info
		- alert-warning
		- alert-danger
	The alert dismisses automaticly after 3 seconds.
*/
function buildAlertMessage(message, messageType) {
	var localizedMessage = localStorage.getItem(message);
	var innerhtml = "<div class=\"container-fluid\"><div class=\"alert alert-" + messageType + " alert-dismissable\"><a href=\"#\" class=\"close\" data-dismiss=\"alert\" aria-label=\"close\">&times;</a>" + localizedMessage + "</div></div>";
	$("#alert-messages").append(innerhtml);
	window.setTimeout(function () {
		$(".alert").fadeTo(500, 0).slideUp(500, function () {
			$(this).remove();
		});
	}, 3000);
}