cr.define('miner', function() {
  'use strict';

  function onAccept(e) {
    chrome.send('handleActivateSignIn');
  }

  function onDecline(e) {
    chrome.send('handleUserDecline');
    e.preventDefault();
  }
  
  function initialize() {
    
    $('accept-button').addEventListener('click', onAccept);
    $('decline-button').addEventListener('click', onDecline);
    
    // $('miner-message')是页面内元素id="miner-message"
    // minerTitle,userName 是c++里变量
    $('miner-message').textContent = loadTimeData.getStringF('minerMessage', loadTimeData.getString('userName'));
    
    // 这几个有固定值的，在c++里面面已经赋过值
    // $('miner-title').textContent = loadTimeData.getString('minerTitle');
    // $('miner-text1').textContent = loadTimeData.getString('minerText1');
    // $('miner-text2').textContent = loadTimeData.getString('minerText2');
    // $('miner-text3').textContent = loadTimeData.getString('minerText3');
    chrome.send('addNumbers', [2, 2]);
  }

  function addResult(result) {
    alert('The result of our C++ arithmetic: 2 + 2 = ' + result);
  }
  
  // Return an object with all of the exports.
  return {
    addResult: addResult,
    initialize: initialize,
  };
});

document.addEventListener('DOMContentLoaded', miner.initialize);
