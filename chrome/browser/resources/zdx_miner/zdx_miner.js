cr.define('miner', function() {
  'use strict';
  
  // sUserID
  // sUUID
  // sMinerYieldRate
  var user_id = "";
  var sInitUserID = "";
  
  function showPopover(target, msg) {
	var res = $.parseJSON(msg);
	if (res.rt == 0) {
		target.attr("data-content", "绑定成功");
	} else {
		target.attr("data-content", "绑定失败:" + res.error);
	}
	target.attr("data-container", "body");
    $('[data-toggle="popover"]').popover();
    target.popover('show');
    target.focus();

    //2秒后消失提示框
    var id = setTimeout(
        function () {
            target.attr("data-content", "");
            target.popover('hide');
        }, 2000
    );
  }

  // function onUserIDSetting(e) {
	// console.log(user_id);
    // chrome.send('setUserID', [user_id]);
	// //chrome.send('getMinerRate', [9, 'xmr', 2]);
	// //chrome.send('getMinerYieldRate', ['xmr', 'cpu', 1]);
	// //chrome.send('getMinerYieldRate', ['xmr', 'agpu', 0]);
	// //chrome.send('getMinerYieldRate', ['xmr', 'ngpu', 0]);
	// //chrome.send('getMinerYieldRate', ['eth', 'cpu', 0]);
	// //chrome.send('getMinerYieldRate', ['eth', 'ngpu', 0]);
  // }
  
	$(document).ready(function(){ 
		// $("#user_id_setting").click(function(){
			// console.log('$("#user_id").text().length(): ' + $("#user_id").text().length());
			// if ($("#user_id").text().length <= 0) {
				// $('#user_id_setting').prop('placeholder', '请设置用户ID(数字类型)');
				// $('#user_id').focus();
				// return;
			// }
			// user_id = $("#user_id").text();
			// if ($("#user_id_setting").text() == "绑定矿机") {
				// chrome.send('setUserID', [user_id]);
			// } else {
				// chrome.send('unsetUserID', [user_id]);
			// }
			
			// chrome.send('setUserID', [user_id]);
		// });
		
		$("#miner_stop").click(function(){
			if ($("#miner_stop").text() == "一键挖矿") {
				chrome.send('setUserID', [user_id]);
			} else {
				chrome.send('minerStop', ['cpu']);
			}
		});
		
		$('#user_id_setting').click(function(){
			// if($('#btn_bind').text() == '绑定'){
				// $('#btn_bind').text('解除绑定');
				// $('#msg').text('绑定成功！');
				// $('#msg').removeClass('alert-danger').addClass('alert-success').show();

			// }else{
				// $('#btn_bind').text('绑定');
				// $('#msg').text('绑定失败！');
				// $('#msg').removeClass('alert-success').addClass('alert-danger').show();
			// }
			
			if ($("#user_id").val().length <= 0) {
				$('#user_id_setting').prop('placeholder', '请设置用户ID(数字类型)');
				$('#user_id').focus();
				return;
			}
			user_id = $("#user_id").val();
			if ($("#user_id_setting").text() == "绑定矿机") {
				chrome.send('setUserID', [user_id]);
			} else {
				chrome.send('minerStop', ['cpu']);
			}
		})
		chrome.send('getUserID', []);
	}); 
  
  function initialize() {
	$(function () {
		$("#user_id").change(function(){
			user_id = this.value;
		});
	});
  }
  
  function setUserIDResult(result) {
	console.log('setUserIDResult = ' + result);
	var res = $.parseJSON(result);
	if (res.rt == "0") {
		if ($("#user_id_setting").text() == "绑定矿机") {
		    $("#user_id_setting").text("解除绑定");
		    $('#msg').text('绑定成功！');
			console.log('setUserIDResult  绑定成功！01');
		} else {
		    $("#user_id_setting").text("绑定矿机");
		    $('#msg').text('解绑成功！');
			console.log('setUserIDResult  解绑成功！01');
		}
		$('#msg').removeClass('hidden').removeClass('alert-danger').addClass('alert-success').show();
	} else {
		if ($("#user_id_setting").text() == "绑定矿机") {
			$('#msg').text('绑定失败！');
			console.log('setUserIDResult  绑定失败！01');
		} else {
			$('#msg').text('解绑失败！');
			console.log('setUserIDResult  解绑失败！01');
		}
		$('#msg').removeClass('hidden').removeClass('alert-success').addClass('alert-danger').show();
	}
  }
  
  function unsetUserIDResult(result) {
	console.log('unsetUserIDResult = ' + result);
	user_id = "";
	
	//showPopover($("#user_id"), result);
  }
  
  function getMinerRateResult(result) {
	console.log('getMinerRateResult = ' + result);
  }
  
  function getMinerYieldRateResult(result) {
	console.log('getMinerYieldRateResult = ' + result);
	if ((user_id != "") || (sInitUserID != "")) {
		$('#miner_yield_rate').text(result);
	}
  }
  
  function getUserIDResult(result) {
	sInitUserID = result;
	if (result != "") {
		$("#user_id").val(result);
		$("#user_id_setting").text("解除绑定");
		// $('#user_id_setting').addClass('btn btn-success');
        // $('#user_id_setting').prop('btn btn-success', true);
		$('#msg').text('绑定成功！');
		$('#msg').removeClass('alert-danger').addClass('alert-success').show();
	} else {
		$("#user_id_setting").text("绑定矿机");
		// $('#user_id_setting').addClass('btn btn-primary');
        // $('#user_id_setting').prop('btn btn-primary', true);
		$('#msg').text('解绑成功！');
		$('#msg').removeClass('alert-danger').addClass('alert-success').show();
	}
  }
  
  function minerStopResult(result) {
	// console.log('minerStopResult = ' + result);
	// var res = $.parseJSON(result);
	// if (res.rt == "0") {
		// if ($("#miner_stop").text() == "一键挖矿") {
			// $('#miner_stop').removeClass('btn-primary');
		    // $('#miner_stop').addClass('btn btn-success');
            // $('#miner_stop').prop('btn btn-success', true);
		    // $("#miner_stop").text("停止挖矿");
		// } else {
			// $('#miner_stop').removeClass('btn-success');
		    // $('#miner_stop').addClass('btn btn-primary');
            // $('#miner_stop').prop('btn btn-primary', true);
		    // $("#miner_stop").text("一键挖矿");
		// }
	// } else {
		
	// }
	setUserIDResult(result);
  }
  
  // Return an object with all of the exports.
  return {
    setUserIDResult: setUserIDResult,
	unsetUserIDResult: unsetUserIDResult,
	getMinerRateResult : getMinerRateResult, 
	getMinerYieldRateResult : getMinerYieldRateResult,
	getUserIDResult: getUserIDResult,
	minerStopResult: minerStopResult,
    initialize: initialize
  };
});

document.addEventListener('DOMContentLoaded', miner.initialize);
