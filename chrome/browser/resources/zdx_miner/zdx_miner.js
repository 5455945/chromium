cr.define('miner', function() {
  'use strict';
  
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
	
    var id = setTimeout(
        function () {
            target.attr("data-content", "");
            target.popover('hide');
        }, 2000
    );
  }
  
  $(document).ready(function(){ 
    $("#miner_stop").click(function(){
      if ($("#miner_stop").text() == "一键挖矿") {
        chrome.send('setUserID', [user_id]);
      } else {
        chrome.send('minerStop', ['cpu']);
      }
    });
    
    $('#user_id_setting').click(function(){
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
		$('#msg').text('绑定成功！');
		$('#msg').removeClass('alert-danger').addClass('alert-success').show();
	} else {
		$("#user_id_setting").text("绑定矿机");
		$('#msg').text('解绑成功！');
		$('#msg').removeClass('alert-danger').addClass('alert-success').show();
	}
  }
  
  function minerStopResult(result) {
	setUserIDResult(result);
  }
  
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
