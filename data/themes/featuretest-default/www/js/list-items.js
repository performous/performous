/*
	Helper functions for lists.
*/
"use strict";

/*
	Clears the given list by remove all child nodes from the DOM.
*/
function clearList(selector) {
	var myNode = document.getElementById(selector);
	while (myNode.firstChild) {
		myNode.removeChild(myNode.firstChild);
	}
}