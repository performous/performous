"use strict";
$(function () {
	$(".header").load("header.html", function(){
		$('.menu').load("menu.html", function(){
			$('.content').load("content.html", function (){
				$('#custom-modal').load("generic-modal.html", function (){
					$('#database').load("database.html", function(){
						$('#playlist').load("playlist.html", function(){
							$('#search').load("search.html", function() {
								$('.footer').load("footer.html", function(){
									$('#custom-scripts').load("site.js", function(){
										console.log("Everything is loaded");
									})
								});
							});
						});			
					});		
				});						
			});		
		});	
	});
});