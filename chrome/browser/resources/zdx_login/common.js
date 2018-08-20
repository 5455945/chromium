let baseApi = "https://zdx.app/api";
let baseWeb = "https://zdx.app";

window.onerror = function(msg, url, line){
    write_debug(msg+"("+url+":"+line+")");
}

function get_auth(param){
	var key_a = [];
	var type = '';

	var date = new Date();
	param['timestamp'] = parseInt(date.getTime()/1000);

	for(var i in param){
		type = typeof(param[i]);
		if('string' == type || 'number' == type || 'boolean' == type){
			key_a.push(i);
		}
	}

	key_a.sort();

	var key_str_a = [];
	var val_str_a = [];
	for(var i in key_a){
		key_str_a.push(key_a[i]);
		val_str_a.push(param[key_a[i]]);
	}

	param['code'] = md5(key_str_a.join('')+val_str_a.join(''));
	return param;
}

//发送api请求
function send_post(url, param, callback){
    if(false == window.navigator.onLine){
        callback({rt:false,'error':'请求失败！请检查网络设置'});
        return false;
    }

	get_auth(param);

    var data = {
        url : url,
        json : true,
        body : param,
        method : "post"
    };

	$.ajax({
		url : url,
		data : param,
		dataType : "json",
		type : "post",
		timeout : 20000,
		success : function(ret){
			callback(ret);
		},error: function(a,b){
			if(a.responseJSON){
				callback(a.responseJSON);
			}else if(false == window.navigator.onLine){
				callback({rt:false,'error':'请求失败！请检查网络设置'});
			}else if(0 == a.status){
				write_debug('请求失败：'+a.status+',错误信息：'+b);
				//httpdns_send_post(url, param, callback);
				httpdns_send_post( data, callback);
			}else{
				callback({rt:false,'error':'请求失败！状态码：'+a.status+',错误信息：'+b});
			}
		}
	});
	
}

function httpdns_send_post(data, callback){
    if("undefined" == typeof(dns)){
        window.dns = require("dns");
    }
    if("undefined" == typeof(URL)){
        window.URL = require("url");
    }
    if("undefined" == typeof(request)){
        window.request = require("request");
    }

    var request_url = data.url;

    var myUrl = new URL(request_url);

    if("undefined" == typeof(data.headers)){
        data.headers = {};
    }
    if("undefined" == typeof(data.strictSSL)){
        data.strictSSL = false;
    }

    dns.setServers( ['114.114.114.114', '8.8.8.8'] );

    write_debug("设置DNS,获取接口解析地址");

    dns.resolve(myUrl.hostname, (err, records) => {
        if(err){
            write_debug('resolve error:'+err);
        }else{
            
            //electron.getGlobal('sharedObject').http_dns_ip = records[0];

            data.headers['host'] = myUrl.host;

            data['url'] = request_url.replace(myUrl.host, records[0]);
        }

        httpdns_request(data, callback);
    })
}

function httpdns_request(data, callback){
    const req = request( data, (error, response, body) => {
        if(error){
            write_debug(data.url+'请求失败:'+JSON.stringify(data.headers)+',错误信息：'+error);
            console.log(data.url+'请求失败:'+JSON.stringify(data.headers)+',错误信息：'+error);
            callback({rt:false,'error':'请求失败!'+error});
        }else if("object" == typeof(body) && "undefined" != typeof(body.rt)){
            callback(body);
        }else if("string" == typeof(body) && "" != body){
            callback({rt:true,'data':body});
        }else{
            callback({rt:false,'error':'请求失败!'+response.statusMessage});
        }
    });
}

//md5值
function md5(string){
    var crypto= require('crypto');  
    var md5_hash = crypto.createHash("md5");  
    md5_hash.update(string);  
    var str = md5_hash.digest('hex');  
    return str.toLowerCase();
}

function gbk2utf8(msg){
    try{
        if('undefined' == typeof(iconv)){
            window.iconv = require('iconv-lite');
        }
        
        return iconv.decode( Buffer.from(msg, 'binary'), 'cp936');
    }catch(e){
        return 'gbk2utf8 error:'+e.message;
    }
}

Date.prototype.Format = function (fmt) { //author: meizz 
    var o = {
        "M+": this.getMonth() + 1, //月份 
        "d+": this.getDate(), //日 
        "h+": this.getHours(), //小时 
        "m+": this.getMinutes(), //分 
        "s+": this.getSeconds(), //秒 
        "q+": Math.floor((this.getMonth() + 3) / 3), //季度 
        "S": this.getMilliseconds() //毫秒 
    };
    if (/(y+)/.test(fmt)) fmt = fmt.replace(RegExp.$1, (this.getFullYear() + "").substr(4 - RegExp.$1.length));
    for (var k in o)
    if (new RegExp("(" + k + ")").test(fmt)) fmt = fmt.replace(RegExp.$1, (RegExp.$1.length == 1) ? (o[k]) : (("00" + o[k]).substr(("" + o[k]).length)));
    return fmt;
}

function storage_pwd(phone_number, passwd){

    var dateObj = new Date();
    var now_time = ''+dateObj.getTime();
    var msg = [phone_number, passwd ? cipher_data(passwd, now_time) : '', now_time];
    localStorage.setItem('REMINDPWD', msg.join('|-|'));
}

function get_pwd(){
    var REMINDPWD = localStorage.getItem('REMINDPWD');
    if("undefined" != typeof(REMINDPWD) && REMINDPWD){
        var msg = REMINDPWD.split("|-|");
        var tmp_passwd = decipher_data(msg[1], msg[2]);

        return [msg[0], tmp_passwd];
    }else{
        return ['',''];
    }
}

function cipher_data(data, key){
    var crypto = require('crypto');
    var algorithm = "undefined" == typeof(arguments[2]) || !arguments[2] ? 'aes192' : arguments[2];

    var encrypted = "";
    try{
        var cip = crypto.createCipher(algorithm, key);
        encrypted += cip.update(data, 'utf8', 'hex');
        encrypted += cip.final('hex');
    }catch(e){
        console.log('cipher error:'+e.message);
    }
    
    return encrypted
}

function decipher_data(data, key){
    var crypto = require('crypto');
    var algorithm = "undefined" == typeof(arguments[2]) || !arguments[2] ? 'aes192' : arguments[2];

    var decrypted = "";
    try{
        var decipher = crypto.createDecipher(algorithm, key);
        decrypted += decipher.update(data, 'hex', 'utf8');
        decrypted += decipher.final('utf8');
    }catch(e){
        console.log('decipher error:'+e.message);
    }
    
    return decrypted
}
