var initial_loaded=false;
var unsaved_data=false;
var lost_connection=false;
var garden_zones=[];
var sent_refresh_request=false;

$(document).ready(function(){
	garden_zones=[svg_21,svg_28,svg_29,svg_30,svg_31,svg_1];
	// Defeat the cache demon. The buttons would stays on even though the html specified disable flag
	lockButtonInterface(true);

	$.ajaxSetup({timeout:3000});

	$( document ).ajaxError(function(){
		lost_connection=true;
		setAjaxError("");
	});
	$(document).ajaxSuccess(function(){
		lost_connection=false;
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

	$("#garden_pause_btn").click(function(){
		if(!initial_loaded) return;	// Wait for the page to be loaded

		$.get("garden_update?mode=toggle_state",function(){
			refreshPageData();
		});
	});

	$("body").on("change keyup","input",function(){
		console.log("asd");
		unsaved_data=true;
		if(!lost_connection) setSaveBtnState(true);
	});
	$(".garden_schedule_list").on("click","#garden_add_schedule",function(){
		if(!initial_loaded) return;	// Wait for the page to be loaded

		$("#garden_list_template>li").clone().insertBefore($(this).closest("li"));
		unsaved_data=true;
		if(!lost_connection) setSaveBtnState(true);
	});
	$("#garden_save").click(function(){
		if(!initial_loaded) return;	// Wait for the page to be loaded

		var get_command="";
		var i=0;
		var error=false;
		$(".garden_schedule_list > li").each(function(el){
			if($(this).attr("data-controls")=="true") return;

			var start=$(this).find(".input_schedule_start").val();
			var end=$(this).find(".input_schedule_end").val();

			// The time inputs are empty. Thus delete element
			if(start.replace(/\s/g)=="" || end.replace(/\s/g)==""){
				$(this).remove();
				return;
			}

			// Get selected components: valves or relays. If none is selected delete element
			var components=getSelectedComponents(this);
			if(components.selected_components.length==0){
				$(this).remove();
				return;
			}

			// Do not remove element if the days are not selected. Perhaps this rule is deactivated
			var week_days=getSelectedWeekDays(this);
			
			start_integer=stringTimeToInt(start);
			end_integer=stringTimeToInt(end);
			if(start_integer==-1 || end_integer==-1) {
				alert("Timpii introdusi sunt malformati: "+start+" <-> "+end);
				error=true;
				return false;
			}
			i++;
			get_command+="start["+i+"]="+start_integer+"&end["+i+"]="+end_integer+"&valve_components["+i+"]="+components.valve_bin_components+"&relay_components["+i+"]="+components.relay_bin_components+"&days["+i+"]="+week_days.bin_days+"&";
		});
		console.log(get_command);

		if(error || i>50) {
			alert("Eroare in timpul salvarii! Nimic nu va fi salvat!");
			return;
		}
		if(i==0) get_command="total=0";
		else get_command="total="+i+"&"+get_command.slice(0,-1);

		console.log(get_command);
		
		$.get("garden_update?mode=set_schedule&"+get_command,function(){
			refreshPageData();
			unsaved_data=false;
			setSaveBtnState(false);
		});
	});
});
function refreshPageData(initial_loading){
	sent_refresh_request=true;
	$.get("garden_refresh?mode=load",function(obj){	
		// current_time,garden_state
		$("#ajax_status").text(obj.current_time);
		$("#garden_pause_btn").text(obj.garden_state==1?"Pauza":"Reporneste");
		colorActiveZones(obj.opened_valves);
	}).always(function(){
		sent_refresh_request=false;
	});
}
function loadPageData(){
	// Load the first info from the server this time only
	sent_refresh_request=true;

	$.get("garden_refresh?mode=load",function(obj){	
		// current_time,garden_state
		initial_loaded=true;

		$("#garden_pause_btn").prop("disabled",false);
		$("#garden_add_schedule").prop("disabled",false);

		$("#ajax_status").text(obj.current_time);
		$("#garden_pause_btn").text(obj.garden_state==1?"Pauza":"Reporneste");

		for(var i=0;i<obj.schedule_list.length;i++){
			var el=$("#garden_list_template>li").clone().insertBefore($("#garden_add_schedule").closest("li"));

			$(el).find(".components_list input[data-type='valve']").each(function(index){
				$(this)[0].checked=(Math.pow(2,index) & obj.schedule_list[i].valve_components);
			});
			$(el).find(".components_list input[data-type='relay']").each(function(index){
				$(this)[0].checked=(Math.pow(2,index) & obj.schedule_list[i].relay_components);
			});
			$(el).find(".week_days_list input").each(function(index){
				$(this)[0].checked=(Math.pow(2,index) & obj.schedule_list[i].active_days);
			});
			$(el).find(".input_schedule_start").val(Math.floor(obj.schedule_list[i].start_time/60)+":"+(obj.schedule_list[i].start_time%60));
			$(el).find(".input_schedule_end").val(Math.floor(obj.schedule_list[i].end_time/60)+":"+(obj.schedule_list[i].end_time%60));
		}
		colorActiveZones(obj.opened_valves);
	}).always(function(){
		sent_refresh_request=false;
	});

}
function colorActiveZones(bin){
	for(var i=0;i<garden_zones.length;i++){
		var mask=Math.pow(2,i);
		if(mask & bin){
			garden_zones[i].style.fill="blue";
		}else garden_zones[i].style.fill="#667856";
	}
}

function getSelectedWeekDays(parent_list_item){
	var selected_days=[];
	var bin_days=0;
	$(parent_list_item).find(".week_days_list .day_state").each(function(){
		if($(this).get(0).checked==true){
			var comp_id=$(this).attr("data-day");
			selected_days.push(comp_id);
			bin_days+=Math.pow(2,parseInt(comp_id));
		}
	});
	return {selected_days:selected_days,bin_days:bin_days};
}
function getSelectedComponents(parent_list_item){
	var selected_components=[];
	var valve_bin_components=0;
	var relay_bin_components=0;

	$(parent_list_item).find(".components_list .component_state").each(function(){
		if($(this).get(0).checked==true){
			var comp_id=$(this).attr("data-component");
			var comp_type=$(this).attr("data-type");
			selected_components.push({comp_type:comp_type,comp_id:comp_id});
			if(comp_type=="valve") valve_bin_components+=Math.pow(2,parseInt(comp_id));
			else relay_bin_components+=Math.pow(2,parseInt(comp_id));
		}
	});
	return {selected_components:selected_components,valve_bin_components:valve_bin_components,relay_bin_components:relay_bin_components};
}
function setSaveBtnState(enable){
	if(enable){
		$("#garden_save").removeAttr("disabled");
	}else $("#garden_save").attr("disabled","");
}
// function changeStateButton(el,disable){
// 	if(disable){
// 		if($(el).prop("disabled")==true) return;
// 		$(el).prop("data-changed-state",true);
// 		$(el).prop("disabled",true);
// 	}else{
// 		if($(el)[0].hasAttribute("data-changed-state")==false) return;
// 		$(el).prop("disabled",false);
// 		$(el)[0].removeAttr("data-changed-state");
// 	}
// }
function lockButtonInterface(lock){
	// $("#garden_pause_btn").prop("disabled",lock);
	// $("#garden_add_schedule").prop("disabled",lock);
	$("#garden_save").prop("disabled",lock);
	if(!unsaved_data && !lock) $("#garden_save").prop("disabled",true);
}
function setAjaxError(msg){
	$("#ajax_status").css("background-color","red");
	lockButtonInterface(true);
}
function clearAjaxError(){
	$("#ajax_status").css("background-color","white");
	lockButtonInterface(false);
}
function stringTimeToInt(str){
	var regex1=/^\s*(\d{1,2})\s*$/g;
	var regex2=/^\s*(\d{1,2})\s*:\s*(\d{1,2})\s*$/g;

	var format1=regex1.exec(str);
	var format2=regex2.exec(str);
	if(format1==null && format2==null) return -1;
	if(format1!=null) return parseInt(format1[1]*60);
	return parseInt(format2[1])*60+parseInt(format2[2]); 
}