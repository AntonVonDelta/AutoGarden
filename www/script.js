var timer_state=2;
var initial_loaded=false;
var sent_refresh_request=false;
var unsaved_data=false;

$(document).ready(function(){
	$.ajaxSetup({timeout:3000});

	$("body").on("change keyup","input",function(){
		unsaved_data=true;
		setSaveBtnState(true);
	});

	$("#timer_update_btn").click(function(){
		var opened_time=parseInt($("#timer_opened_time").val()); 
		var closed_time=parseInt($("#timer_closed_time").val()); 

		if(opened_time>=255 || closed_time>=255){
			alert("Valori prea mari");
			return;
		}

		// ESP32 receives seconds. (*60) will transform the input(which are minutes) into seconds
		$.get("timer_update?mode=time&opened_time="+(opened_time*60)+"&closed_time="+(closed_time*60),function(){
			unsaved_data=false;
			setSaveBtnState(false);
			refreshPageData();
		});
	});

	$("#timer_start_btn").click(function(e){
		if(e.target.id!=="timer_start_btn") return;
		var time=parseInt($("#timeouts").val());		
		
		if(timer_state==2) return;	// Timer is paused

		$.get("timer_start?time="+(time*60),function(){
			refreshPageData();
		});
		
	});
	$("#timer_pause_btn").click(function(){				
		$.get("timer_update?mode=state_change",function(){
			refreshPageData();
		});
	});
	
	$( document ).ajaxError(function(){
		setAjaxError("");
	});
	$(document).ajaxSuccess(function(){
		clearAjaxError();
	});
	

	function makeRequest(){
		if(sent_refresh_request) return;
		if(initial_loaded) refreshPageData();
		else loadPageData();
	}
	function loop(){
		makeRequest();
		setTimeout(loop,3000);
	}
	loop();
	// setInterval(function(){
	// 	if(sent_refresh_request) return;
	// 	if(initial_loaded) refreshPageData();
	// 	else loadPageData();
	// },3000);
});


function loadPageData(){
	sent_refresh_request=true;
	$.get("timer_refresh",function(obj){
		initial_loaded=true;

		// history, output_state, remaining, secondary_remaining, current_time, timer_state
		$("#timer_state").text(obj.output_state==0?"Deschis":"Inchis");
		$("#ajax_status").text(obj.current_time);

		$("#timer_opened_time").val(Math.floor(obj.opened_time/60));
		$("#timer_closed_time").val(Math.floor(obj.closed_time/60));

		$("#timer_remaining").text(Math.floor(obj.remaining/60)+":"+(obj.remaining%60));
		$("#timer_primary_next_state").text(obj.primary_output_state==1?"Deschidere":"Inchidere");	// This should print the future so we inverse the codes( ==1 instead of 0)
		$("#timer_secondary_remaining").text(Math.floor(obj.secondary_remaining/60)+":"+(obj.secondary_remaining%60));

		timer_state=obj.timer_state;

		var button_text="";
		if(obj.timer_state==0) button_text="Pauza";
		else if(obj.timer_state==1) button_text="Revoca contorul";
		else button_text="Reporneste";

		$("#timer_pause_btn").text(button_text);
	}).always(function(){
		sent_refresh_request=false;
	});
}
function refreshPageData(){
	sent_refresh_request=true;
	$.get("timer_refresh",function(obj){
		// history, output_state, remaining, secondary_remaining, current_time, timer_state
		$("#timer_state").text(obj.output_state==0?"Deschis":"Inchis");
		$("#ajax_status").text(obj.current_time);

		$("#timer_remaining").text(Math.floor(obj.remaining/60)+":"+(obj.remaining%60));
		$("#timer_primary_next_state").text(obj.primary_output_state==1?"Deschidere":"Inchidere");	// This should print the future so we inverse the codes( ==1 instead of 0)
		$("#timer_secondary_remaining").text(Math.floor(obj.secondary_remaining/60)+":"+(obj.secondary_remaining%60));

		timer_state=obj.timer_state;

		var button_text="";
		if(obj.timer_state==0) button_text="Pauza";
		else if(obj.timer_state==1) button_text="Revoca contorul";
		else button_text="Reporneste";

		$("#timer_pause_btn").text(button_text);
	}).always(function(){
		sent_refresh_request=false;
	});
}



function setSaveBtnState(enable){
	if(enable){
		$("#timer_update_btn").removeAttr("disabled");
	}else $("#timer_update_btn").attr("disabled","");
}
function setAjaxError(msg){
	$("#ajax_status").css("background-color","red");
}
function clearAjaxError(){
	$("#ajax_status").css("background-color","white");
}