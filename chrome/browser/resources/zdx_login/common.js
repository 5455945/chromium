//let userDataDir = app.getPath("userData");
let baseApi = "https://zdx.app/api";
let baseWeb = "https://zdx.app";
// let flash_path = "";
// try{
//     flash_path = app.getPath('pepperFlashSystemPlugin');
// }catch(e){
    
// }
global.sharedObject = {
    isAutoUpdate : false,
    isDebug : false,
    trusted_all_url : false,
    trusted_url : [],
    http_dns_ip : "",
    default_url : null,
    exe : app.getPath("exe"),
    download_path : PATH.join(app.getPath("downloads"), "/"),
    flash_path : "",
    protocol : 'browser',
    code_path : PATH.join(__dirname, '/'),
    files : {
        cookies : PATH.join( userDataDir, "Cookies"),
        history : PATH.join( userDataDir, "History"),
        favicons : PATH.join( userDataDir, 'Favicons'),
        historys : PATH.join(userDataDir, 'Historys'),
        bookmarks : PATH.join( userDataDir, "Bookmarks_e"),
        downloads : PATH.join( userDataDir, "Downloads"),
        settings : PATH.join( userDataDir, "Settings"),
        lastTabs : PATH.join( userDataDir, "Last Tabs")
    },
    default_bookmarks : [
        {hostname:"www.huobi.pro", url:"https://www.huobi.pro/zh-cn/",title:"火币"},
        {hostname:"www.binance.com", url:"https://www.binance.com/",title:"币安"},
        {hostname:"www.okex.com",url:"https://www.okex.com/",title:"OKEX"},
        {hostname:"www.bitfinex.com",url:"http://www.bitfinex.com/",title:"Bitfinex"},
        {hostname:"www.fcoin.com",url:"https://www.fcoin.com/", title:"FCOIN"},
        {hostname:"www.bifei.app",url:"https://www.bifei.app/",title:"币飞"}
    ],
    work_path : PATH.join(__dirname+'/../'),
    api : {
        code : baseApi+"/v1/message/code",  //发送验证码
        login : baseApi+"/v1/member/login", //登录接口
        register : baseApi+"/v1/reg/index", //注册接口
        reset : baseApi+"/v1/member/reset", //重置密码
        logout : baseApi+"/v1/member/logout",   //退出登录
        'check-token' : baseApi+"/v1/member/check-token",   //用户信息
        info : baseApi+"/v1/member/info",
        deviceReg : baseApi + '/v1/device/reg',
        update : baseApi + '/v1/desktop/update',
        'realtime-income' : baseApi + "/v1/member/realtime-income",
        'vat-income' : baseApi + "/v1/device/vat-income"
    },
    web : {
        profile : baseWeb+"/member/profile",
        about : baseWeb+"/about"
    },
    ws : {
        rate_report : "wss://zdx.app/socket"
    },
    passwd_prefix : "jraZXF0ShigeXcRb",
};
global.sharedObject.emptyTab = global.sharedObject.protocol+"://empty";

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

    if("" != electron.getGlobal('sharedObject').http_dns_ip){
        httpdns_send_post( data, callback);
    }else{
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

//写日志
function write_debug(msg){
    var dateObj = new Date();
    nowDate = dateObj.Format('yyyy-MM-dd hh:mm:ss');

    if(typeof(fs) == 'undefined'){
        window.fs = require('fs');
    }
    if('undefined' == typeof(os)){
		window.os = require('os');
	}

    fs.writeFile(electron.getGlobal('sharedObject').work_path+'/debug.log', '['+nowDate+'] ' + msg + os.EOL, {encoding:'utf8',flag:'a'}, function(err){
        if(err){
            console.log('write debug error :'+err);
        }
    });
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

async function get_hd_uuid(){
    if("undefined" != typeof(shareVar['uuid']) && shareVar['uuid']){
        return shareVar['uuid'];
    }

    if("undefined" == typeof(si)){
        window.si = require('systeminformation');
    }

    try{
        var data = await si.system(); 
        shareVar['uuid'] = data.uuid;
        return data.uuid;
    }catch(e){
        write_debug('get hd uuid error :'+e);
        console.log("get hd uuid error:"+e);
        return "";
    }
    
}

function get_update(){
    if("undefined" == typeof(app)){
        window.app = electron.app;
    }

    var param = {
        type : electron.getGlobal('sharedObject').update_type,
        version : electron.app.getVersion()
    };

    window.isManual = 'undefined' != typeof(arguments[0]) ? arguments[0] : false;

    send_post(electron.getGlobal('sharedObject').api.update , param, get_update_end);
}

function get_update_end(ret){
    if(!ret['rt']){
        if(isManual){
            electron.dialog.showMessageBox({type:"info",title:'检查更新',message:'当前无更新版本'});
        }
        return;
    }

    if('object' != typeof(ret['data']) || "undefined" == typeof(ret['data']['version']) || "" == ret['data']['version']){
        if(isManual){
            electron.dialog.showMessageBox({type:"info",title:'检查更新',message:'当前版本：'+electron.app.getVersion()+"已是最新版本"});
        }
        return;
    }

    var version_a = ret['data']['version'].split('.');
    var version_i = 0;
    var version_c = version_a.length;

    for(var i = version_c; i > 0; i--){
        version_i += parseInt(version_a[version_c - i]) * Math.pow(10,i); 
    }

    var cur_version_a = app.getVersion().split(".");
    var cur_version_i = 0;
    version_c = cur_version_a.length;
     for(var i = version_c; i > 0; i--){
        cur_version_i += parseInt(cur_version_a[version_c - i]) * Math.pow(10,i); 
    }

    if(cur_version_i >= version_i){
        if(isManual){
            //delete isManual;
            electron.dialog.showMessageBox({type:"info",title:'检查更新','detail':'当前已是最新版本:'+electron.app.getVersion()+"  "+os.arch()});
        }
        return;
    }else if("object" == typeof(shareVar['update']) && "object" == typeof(shareVar['update']['info']) 
        && shareVar['update']['info']['url'] == ret['data']['url'] && shareVar['update']['info']['md5_str'] == ret['data']['md5_str']){
        console.log('is download update');
        if(isManual){
            electron.dialog.showMessageBox({type:"question",title:"检查更新","detail":"当前版本："+electron.app.getVersion()+"  "+os.arch()+"\n最新版本："+ret['data']['version']+"\n"+ret['data']['memo'],buttons:['立即升级','忽略'],defaultId:0}, function(response){
                if(0 == response){
                     exec_update(shareVar['update']['info']['file_name']);
                }else{
                    //delete isManual;
                }
            });
        }
        return;
    }

    if(isManual){
        electron.dialog.showMessageBox({type:"question",title:"检查更新","detail":+electron.app.getVersion()+"  "+os.arch()+"\n最新版本："+ret['data']['version']+"\n"+ret['data']['memo'],buttons:['立即下载更新','忽略'],defaultId:0}, function(response){
            if(0 == response){
                var url_a = ret['data']['url'].split("/");
                var file_name = electron.getGlobal('sharedObject').download_path + url_a[url_a.length - 1];

                var param = {
                    filename :  url_a[url_a.length - 1],
                    filepath : electron.getGlobal('sharedObject').download_path,
                    bytes : (ret['data']['size']*1024*1024),
                    url : ret['data']['url'],
                    callback : function(info){
                        console.log('downloadsAPI add callback :'+info.id);

                        download_file(info, function(result){
                            shareVar['manualUpateUrl'] = ret['data']['url'];

                            if("undefined" == typeof(result) || "undefined" == typeof(result['status']) || 2 != result['status']){
                                write_debug('检查更新，文件下载失败：'+result['error']);
                                return;
                            }

                            fs.readFile(file_name, (err, data)=>{
                                if(err){
                                    electron.dialog.showErrorBox('检查更新','新版本文件读取失败:'+err);
                                    return;
                                }
                                var file_md5 = md5(data); 
                                if(file_md5 != ret['data']['md5_str']){
                                    electron.dialog.showErrorBox('检查更新','新版本文件校验失败');
                                    return;
                                }
                                exec_update(file_name);
                            });

                        });
                    }
                };

                downloadsAPI.add(param);

                 //require("child_process").spawn(shareVar['update']['info']['file_name']);
            }else{
                //delete isManual;
            }
        });
    }

    if(2 == ret['data']['status']){
        console.log('-------------download update ignore');
        return;
    }
    console.log('-------------download update start');
    download_update(ret);

}

function download_update(ret){
    if("undefined" == typeof(request)){
        window.request = nodeRequire('request');
    }
    if("undefined" == typeof(fs)){
        window.fs = nodeRequire("fs");
    }

    var dowloadStatusCode = 0;
    var url_a = ret['data']['url'].split("/");
    var file_name = PATH.join(electron.getGlobal('sharedObject').download_path , "zdx-auto-update.exe");//+url_a[url_a.length - 1];
    var writeStream = fs.createWriteStream(file_name);

    window.requestObj = request.get({url:encodeURI(ret['data']['url']),strictSSL:false,timeout:60000});

    console.log(requestObj);
    requestObj.on('error', function(err) {
        //console.log(err);
        dowloadStatusCode = 0;
        write_debug('检查更新,文件下载失败：'+err);
        if(isManual){
            electron.dialog.showErrorBox('检查更新','文件下载失败：'+err);
        }
    })
    .on('response',function(response){

        dowloadStatusCode = response.statusCode;
        if(200 != dowloadStatusCode){
            write_debug('文件下载失败：'+dowloadStatusCode+'-->'+response.statusMessage);
        }
        
        if(200 == dowloadStatusCode && "undefined" != typeof(response.headers) && response.headers){
            var param = response.headers;
            param['file_name'] = url_a[url_a.length - 1];
        }
    })
    .on('end',function(msg){
        if(dowloadStatusCode != 200){
            write_debug('检查更新,文件下载结束,失败:'+msg);
            
            fs.unlink( file_name,function(err){
                if(err){
                    console.log('unlink error :'+err);
                }
            });

            isManual && dowloadStatusCode && electron.dialog.showErrorBox('检查更新', '文件下载失败');

            return ;
        }

        if(requestObj){

            write_debug('更新文件下载成功');
            fs.readFile(file_name, (err, data)=>{
                if(err){
                    console.log('更新文件读取失败');
                    return;
                }
                var file_md5 = md5(data);
                if(file_md5 != ret['data']['md5_str']){
                    console.log('更新文件校验失败');
                    return;
                }

                shareVar['update']['info'] = {
                    url : ret['data']['url'],
                    file_name : file_name,
                    md5_str : ret['data']['md5_str'],
                    is_update : false
                };

                if("undefined" != typeof(shareVar['manualUpateUrl']) && shareVar['manualUpateUrl'] == shareVar['update']['info']['url']){
                    //已手动执行更新得情况
                    return;
                }

                electron.dialog.showMessageBox({type:"question",title:"检查更新",detail:"检测到新版本，是否立即更新?",buttons:['立即升级','忽略'],defaultId:0}, function(response){
                    if(0 == response){
                        exec_update(file_name, true);
                        return;
                    }
                    
                });

                 
            });

            //electron.dialog.showMessageBox({});
            //setup_update(setup_update_end);
        }
    }).
    on('data',function(data){
        
    })
    .pipe(writeStream)
}



