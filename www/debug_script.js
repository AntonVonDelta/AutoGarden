$(function(){
    var txt="";
    var sent_request=false;
    $.ajaxSetup({timeout:3000});

    $("button").click(function(){
        txt=$("#i").val();
    });

    function loop(){
        if(!sent_request){
            sent_request=true;
            c=(txt!=""?"?c="+txt:"");
            txt="";
    
            $.get("debug_update"+c,function(data){
                $("#o").val(data);
            }).always(function(){
                sent_request=false;
            });
        }

        setTimeout(loop,2500);
    }

    loop();
});