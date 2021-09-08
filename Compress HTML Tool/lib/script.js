$(document).ready(function(){
  $('#a').bind('input propertychange', function() {
    auto_detect();
  });
});
 
function auto_detect(){
  var txt=$("#a").val();

  if(txt.includes("function") || txt.includes("var ") || txt.includes("$(document)")){
    executeJS();
    $("#c").text("Detected JS");
  }else if(txt.includes("<html>") && txt.includes("<body>")){
    executeHTML();
    $("#c").text("Detected HTML");
  }else {
    executeCSS();
    $("#c").text("Detected CSS");
  }
}


function executeHTML(){
  var txt=$("#a").val();

  txt=txt.replace(/\t/g,"");
  txt=txt.replace(/style=\"\s*\"/g,"");
  txt=txt.replace(/>(\s*)</g,"><");
  txt=txt.replace(/\\/g,"\\\\");
  txt=txt.replace(/\"/g,"\\\"");
  txt=txt.replace(/\r\n/g,"");
  txt=txt.replace(/\n/g,"");
  txt=txt.replace(/<!--([\s\S]*?)-->/gm,"");
  $("#b").val(txt);
}

function executeCSS(){
  var txt=$("#a").val();

  txt=txt.replace(/\/\*[\s\S]*?\*\//g,"");
  txt=txt.replace(/{\s*/g,"{");
  txt=txt.replace(/;\s*/g,";");
  txt=txt.replace(/}\s*/g,"}");
  txt=txt.replace(/\r\n/g,"");
  txt=txt.replace(/\n/g,"");
  $("#b").val(txt);
}

function executeJS(){
  var txt=$("#a").val();

  txt=txt.replace(/\/\/.*/g,"");
  txt=txt.replace(/{\s*/g,"{");
  txt=txt.replace(/;\s*/g,";");
  txt=txt.replace(/}\s*/g,"}");
  txt=txt.replace(/\\/g,"\\\\");
  txt=txt.replace(/\"/g,"\\\"");
  txt=txt.replace(/\r\n/g,"");
  txt=txt.replace(/\n/g,"");
  txt=txt.replace(/console\.log\(.+?\)[;]?/g,"");
  txt=txt.replace(/false/g,"!1");
  txt=txt.replace(/(?<!")true(?!")/g,"!0");
  $("#b").val(txt);
}