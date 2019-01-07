<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<?php
header("Cache-Control: no-store, no-cache, must-revalidate");
header("Cache-Control: post-check=0, pre-check=0", false);
header("Last-Modified: " . gmdate("D, d M Y H:i:s") . " GMT");
?>
<meta http-equiv="cache-control" content="no-cache">
<meta http-equiv="expires" content="-1">
<meta http-equiv="pragma" content="no-cache">

<title>OpenAirEmulation Scenario Descriptor - Basic Scenario</title>
<link type="text/css" rel="stylesheet" media="screen,projection" href="/OSD/css/screen.css?8" />
<link type="text/css" rel="stylesheet" media="screen,projection" href="/OSD/css/blueprint.css?8" />
<link type="text/css" rel="stylesheet" media="screen,projection" href="/OSD/css/style.css?8" />
<script type="text/javascript" src="/OSD/jquery/jquery.js?8"></script>
	<!--[if lt IE 7]>
		<link type="text/css" rel="stylesheet" media="all" href="/OSD/css/fix-ie.css" />
	<![endif]-->


<script type="text/javascript">                                         
$(function(){
    $("#envoyer").click(function(){
		valid = true;
	// text non vide
	//        $("input[value=]").filter(".required").css("border-color","#FF0000").next(".error-message").fadeIn("slow").text("Champ vide");
	//        $("input[value!=]").filter(".required").css("border-color","#CCFF00").next(".error-message").fadeOut("fast").next(".good-message").fadeIn("slow");
    $("input[value=]").filter(".required").next(".message").text("Empty").addClass("error-message").removeClass("good-message").fadeIn("slow");
    $("input[value!=]").filter(".required").next(".message").text("Ok").addClass("good-message").removeClass("error-message").fadeIn("slow");
    valid = valid && ($("input[value=]").filter(".required").size() == 0)
	
	// test intger
	//.....
	//  valid = valid and ....
		return valid;
	});
	
	// PART 1 - BUTTON FADING - SMALL SCALE
	$(":radio[name='smallscale']").click(function(){ 
		if ($(":radio[name='smallscale']:checked").val() == 'rayleigh') {
			$("#rayleigh").show(1);
			$("#rician").hide(1);
		}	
		if ($(":radio[name='smallscale']:checked").val() == 'rician') {
			$("#rayleigh").hide(1);
			$("#rician").show(1);
		}
	});   
	
	
	// PART 2 - BUTTON eNB TOPOLOGY  
	$(":radio[name='eNB_initial_distribution']").click(function(){ 
	    if ($(":radio[name='eNB_initial_distribution']:checked").val() == 'random') {
            $("#grid").hide(1);
            $("#hexagonal").hide(1);
            $("#random").hide(1);
            $("#fixed").show(1);
	    }
	    if ($(":radio[name='eNB_initial_distribution']:checked").val() == 'random') {
            $("#grid").hide(1);
            $("#hexagonal").hide(1);
            $("#random").show(1);
            $("#fixed").hide(1);
	    }
	    if ($(":radio[name='eNB_initial_distribution']:checked").val() == 'grid') {
            $("#grid").show(1);
            $("#hexagonal").hide(1);
            $("#random").hide(1);
            $("#fixed").hide(1);
	    }       
	    if ($(":radio[name='eNB_initial_distribution']:checked").val() == 'hexagonal') {
            $("#grid").hide(1);
            $("#hexagonal").show(1);
            $("#random").hide(1);
            $("#fixed").hide(1);
	    }
	});
	
	// PART 2 - BUTTON UE DISTRIBUTION
	$(":radio[name='uedistr']").click(function(){ 
		if ($(":radio[name='uedistr']:checked").val() == 'random') {
			$("#gridue").hide(1);
		}	
		if ($(":radio[name='uedistr']:checked").val() == 'concentrated') {
			$("#gridue").hide(1);
		}
		if ($(":radio[name='uedistr']:checked").val() == 'grid_map') {
			$("#gridue").show(1);
		}
	});   
	$(":radio[name='gridmapradio']").click(function(){ 
		if ($(":radio[name='gridmapradio']:checked").val() == 'random_grid') {
			$("#random_grid").show(1);
			$("#border_grid").hide(1);
		}	
		if ($(":radio[name='gridmapradio']:checked").val() == 'border_grid') {
			$("#random_grid").hide(1);
			$("#border_grid").show(1);
		}
	});   
	
	// PART 2 - BUTTON MOBILITY  
	$(":radio[name='uemobility']").click(function(){ 
		if ($(":radio[name='uemobility']:checked").val() == 'fixed') {
			$("#grid_walk").hide(1);
		}	
		if ($(":radio[name='uemobility']:checked").val() == 'random_waypoint') {
			$("#grid_walk").hide(1);
		}
		if ($(":radio[name='uemobility']:checked").val() == 'random_walk') {
			$("#grid_walk").hide(1);
		}
		if ($(":radio[name='uemobility']:checked").val() == 'grid_walk') {
			$("#grid_walk").show(1);
			
		}
	}); 
	
	// BUTTON TRAFFIC  - PACKET SIZE
	$(":radio[name='pktsize']").click(function(){ 
		if ($(":radio[name='pktsize']:checked").val() == 'fixed') {
			$("#pktsizefixed").show(1);
			$("#pktsizeuniform").hide(1);
		}	
		if ($(":radio[name='pktsize']:checked").val() == 'uniform') {
			$("#pktsizefixed").hide(1);
			$("#pktsizeuniform").show(1);
		}
	});   
	
	// BUTTON TRAFFIC  - INTER ARRIVAL TIME
	$(":radio[name='interarrivtime']").click(function(){ 
		if ($(":radio[name='interarrivtime']:checked").val() == 'fixed') {
			$("#intarrtimefixed").show(1);
			$("#intarrtimeuni").hide(1);
			$("#intarrtimepoisson").hide(1);
		}	
		if ($(":radio[name='interarrivtime']:checked").val() == 'uniform') {
			$("#intarrtimefixed").hide(1);
			$("#intarrtimeuni").show(1);
			$("#intarrtimepoisson").hide(1);
		}
		if ($(":radio[name='interarrivtime']:checked").val() == 'poisson') {
			$("#intarrtimefixed").hide(1);
			$("#intarrtimeuni").hide(1);
			$("#intarrtimepoisson").show(1);
		}
	}); 
	
	$("input.disabled").attr("disabled", true);
	
	// PART 4  - RANDOM SEED
	$(":radio[name='randomseed']").click(function(){ 
		if ($(":radio[name='randomseed']:checked").val() == 'random') {
			$("#userspecified").hide(1);
		}	
		if ($(":radio[name='randomseed']:checked").val() == 'userspecified') {
			$("#userspecified").show(1);
		}
	});   
	
});   
</script>                                  
