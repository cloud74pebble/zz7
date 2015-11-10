var appMessageQueue = {
  queue: [],
  numTries: 0,
  maxTries: 5,
  working: false,
  clear: function() {
    this.queue = [];
    this.working = false;
  },
  isEmpty: function() {
    return this.queue.length === 0;
  },
  nextMessage: function() {
    return this.isEmpty() ? {} : this.queue[0];
  },
  send: function(message) {
    if (message) this.queue.push(message);
    if (this.working) return;
    if (this.queue.length > 0) {
    this.working = true;
    var ack = function() {
      appMessageQueue.numTries = 0;
      appMessageQueue.queue.shift();
      appMessageQueue.working = false;
      appMessageQueue.send();
    };
    var nack = function() {
      appMessageQueue.numTries++;
      appMessageQueue.working = false;
      appMessageQueue.send();
    };
    
    if (this.numTries >= this.maxTries) {
      console.log('Failed sending AppMessage: ' + JSON.stringify(this.nextMessage()));
      ack();
    }

    console.log('Sending AppMessage: ' + JSON.stringify(this.nextMessage()));
    Pebble.sendAppMessage(this.nextMessage(), ack, nack);
    }
  }  
};

Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('showConfiguration', function() {
  var url = 'https://rawgit.com/cloud74pebble/zz7/master/config/zz7config.html';
  console.log('Showing configuration page: ' + url);

  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  var configData = JSON.parse(decodeURIComponent(e.response));
  console.log('Configuration page returned: ' + JSON.stringify(configData));

  var dict = {};
    dict['KEY_VIBE_BT'] = configData['vibe_bt'];
    dict['KEY_VIS_DATE'] = configData['visibility_date'];
    dict['KEY_VIS_DOW'] = configData['visibility_dow'];
    dict['KEY_VIS_BAT'] = configData['visibility_bat'];

  console.log('dict: ' + JSON.stringify(dict));
  
  // Send to watchface
  //appMessageQueue.send(dict); 
  var transactionId = Pebble.sendAppMessage(dict, function(e) {
                console.log('Successfully delivered message with transactionId=' + e.data.transactionId);
            }, function(e) {
                console.log('Unable to deliver message with transactionId=' + e.data.transactionId + ' Error is: ' + e.data.error);
                for(var x in e.data) {
                    // Using this technique, I found that e.data is the only property on the object 'e'.
                    console.log('e.data.' + x + ' = ' + e.data[x]);
                }
            });
});