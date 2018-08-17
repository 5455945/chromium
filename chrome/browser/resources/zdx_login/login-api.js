
function show_msg( msg, rt){
	$("#msg").html(msg);
console.log(msg);
	if("undefined" != typeof(rt) && rt){
		$("#msg").css("color", "green");
	}else{
		$("#msg").css("color", "red");
	}
}

function set_clock(){
	clock_t--;
	if(clock_t <= 0){
		$(".validCode[data-sort="+validSort+"]").attr("disabled", false);
		$(".validCode[data-sort="+validSort+"]").html("获取");
	}else{
		$(".validCode[data-sort="+validSort+"]").html(clock_t + "s");
		setTimeout(set_clock, 1000);
	}
}

function get_code(sort, phone_number){
	var param = {
		phone_number : phone_number,
		sort: sort
	};
	if(!param['phone_number']){
		show_msg("请输入手机号码");
		return false;
	}
	show_msg("");

	$(".validCode[data-sort="+validSort+"]").attr("disabled", true);
	$(".validCode[data-sort="+validSort+"]").html("获取中");

	send_post( electron.getGlobal('sharedObject').api.code , param, get_code_end);
}

function get_code_end(ret){
	if(ret['rt']){
		clock_t = 60;
		$(".validCode[data-sort="+validSort+"]").attr("disabled", true);
		set_clock();
	}else{
		$(".validCode[data-sort="+validSort+"]").attr("disabled", false);
		$(".validCode[data-sort="+validSort+"]").html("获取");
		show_msg(ret['error']);
	}
}

function login() {
	var param = {
		'type'	:	'desktop'
	};
	if('' == $("#loginMobile").val()){
		show_msg( "请输入手机号码或邮箱地址进行登录");
		return false;
	}

	if($("#loginMobile").val().indexOf("@") > 0 ){
		param['email'] = $("#loginMobile").val();
		param['passwd'] = md5(electron.getGlobal('sharedObject').passwd_prefix+$("#loginPwd").val());
	}else if($("#loginPwd").is(":visible")){
		param['phone_number'] = $("#loginMobile").val();
		param['passwd'] = md5(electron.getGlobal('sharedObject').passwd_prefix+$("#loginPwd").val());

		storage_pwd(param['phone_number'], $("#loginPwd").val());
	}else{
		param['phone_number'] = $("#loginMobile").val();
		param['valid_code'] = $("#loginCode").val();
	}

	$("#loginBtn").attr("disabled",true);
	$("#loginBtn").html("登录中...");
	show_msg("");

	send_post(electron.getGlobal('sharedObject').api.login, param, login_end);
    return false;
}

function login_end(ret){
	console.log(ret);
	$("#loginBtn").attr("disabled",false);

	if(ret['rt']){
		var last_user_id = localStorage.getItem('user_id');
        
        sessionStorage.setItem('token', ret['data']['token']);
        localStorage.setItem('token', ret['data']['token']);
        localStorage.setItem('user_id', ret['data']['user_id']);
        if("object" == typeof(ret['data']['mine_addr'])){
        	localStorage.setItem('miner_addr', JSON.stringify(ret['data']['mine_addr']));
        }

        localStorage.removeItem('yilu_user_device_id');

        // if("undefined" != typeof(last_user_id) && last_user_id != ret['data']['user_id']){
        // 	localStorage.setItem('last_user_id', last_user_id);
        	
        // }

        window.close();
	}else{
		$("#loginBtn").val("登录");
		show_msg(ret['error']);
	}
}


function register(){
	if('' == $("#registerPwd1").val()){
		show_msg("请输入密码");
		return false;
	}else if('' == $("#registerPwd2").val()){
		show_msg("请输入确认密码");
		return false;
	}else if($("#registerPwd1").val() != $("#registerPwd2").val()){
		show_msg("两次输入密码不一致");
		return false;
	}else if(!chk_pwd($("#registerPwd1").val())){
        show_msg("密码只能包含数字字母!@#$%,长度为6到20位");
        return false;
    }else if('' == $("#registerCode").val()){
    	show_msg("请输入验证码");
    	return false;
    }

    // if('' != $("#invite_id").val() && isNaN($("#invite_id").val())){
    // 	$("#msg").html('推荐人ID输入有误').css('color','red');
    // 	return false;
    // }
    
    show_msg("");

	var param = {
		phone_number : $("#registerMobile").val(),
		valid_code : $("#registerCode").val(),
		passwd : md5(electron.getGlobal('sharedObject').passwd_prefix+$("#registerPwd1").val()),
		// email : $("#email").val(),
		// invite_id : $("#invite_id").val()
	}

	if('' != $("#registerName").val()){
		param['username'] = $("#registerName").val();
	}
	storage_pwd(param['phone_number'], $("#registerPwd1").val());

	$("#registerBtn").attr("disabled", true);
	$("#registerBtn").html("注册中...");
	send_post( electron.getGlobal('sharedObject').api.register, param, register_end);
}

function register_end(ret){
	$("#registerBtn").attr("disabled", false);
	$("#registerBtn").html("注册");

	if(ret['rt']){
		show_msg("注册成功", true);
		//$(".login").click();

		// $("#msg").html("注册成功").css("color","green");
		setTimeout( function(){
            var param = {
                type  :   'desktop',
                phone_number : $("#registerMobile").val(),
                passwd : md5(electron.getGlobal('sharedObject').passwd_prefix+$("#registerPwd1").val())
            };
            send_post( electron.getGlobal('sharedObject').api.login, param, login_end);
		}, 500);
	}else{
		
		show_msg(ret['error']);
	}
}

function reset_pwd(){
    if("" == $("#resetMobile").val()){
    	show_msg('请输入手机号');
    	return false;
    }
    if('' == $("#resetPwd1").val()){
        show_msg("请输入新密码")
        return false;
    }else if('' == $("#resetPwd2").val()){
        show_msg("请输入确认密码");
        return false;
    }else if($("#resetPwd1").val() != $("#resetPwd2").val()){
        show_msg("两次输入密码不一致");
        return false;
    }else if(!chk_pwd($("#resetPwd1").val())){
        show_msg("密码只能包含数字字母!@#$%,长度为6到20位");
        return false;
    }
    show_msg("");

    var param = {
        phone_number : $("#resetMobile").val(),
        valid_code : $("#resetCode").val(),
        passwd : md5(electron.getGlobal('sharedObject').passwd_prefix + $("#resetPwd1").val()),
    };

    $("#resetBtn").attr("disabled",true);
    $("#resetBtn").attr("密码重置中...");

    send_post( electron.getGlobal('sharedObject').api.reset, param, reset_pwd_end);
}

function reset_pwd_end(ret){
	$("#resetBtn").attr("disabled",false);
	$("#resetBtn").html("重置密码");

    if(ret['rt']){
    	show_msg("重置成功", true);
        $(".login").click();
    }else{
    	show_msg(ret['error']);
    }
}


function chk_pwd(passwd){
    var reg = /^[A-Za-z0-9_\-\?\.!@#$%,]{6,20}$/;
    if(!reg.test(passwd)){
        return false;
    }
    return true;

}