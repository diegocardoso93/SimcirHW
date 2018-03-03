var aedes = require('aedes')()
var server = require('net').createServer(aedes.handle)

const WebSocket = require('ws');
const express = require('express');
const path = require('path');
const app = express();
const serverws = require('http').createServer();

Server = {};
Server.startServer = function() {
/*
  app.use(express.static(path.join(__dirname, '/')));

  const wss = new WebSocket.Server({server: server});

  wss.on('connection', function (ws) {

    ws.on('message', function incoming(data) {
      wss.clients.forEach(function each(client) {
        if (client !== ws && client.readyState === WebSocket.OPEN) {
          client.send(data);
        }
      });
    });

  });

  server.on('request', app);
  server.listen(9061, function () {
    console.log('Listening on http://localhost:9061');
  });

*/

  app.use(express.static(path.join(__dirname, '/')));

  var wss = new WebSocket.Server({server: serverws});
  wss.on('connection', (ws) => {
    ws.on('message', (data) => {
      console.log(data)
      aedes.publish({
        cmd: 'publish',
        qos: 0,
        topic: JSON.parse(data).type,
        payload: new Buffer(data),
        retain: false
      }, function() { console.log("ok"); });
    });
  });

  serverws.on('request', app);
  serverws.listen(9062, function () {
    console.log('Listening on http://localhost:9062');
  });


  server.listen(1883, function () {
    console.log('server listening on port', 1883)
  });

  aedes.authorizePublish = function (client, packet, callback) {
    wss.clients.forEach(function each(client) {
      if (client.readyState === WebSocket.OPEN) {
        client.send(packet.payload.toString());
      }
    });

    if (packet.topic === 'bbb') {
      packet.payload = new Buffer('overwrite packet payload')
    }

    callback(null)
  }

}
module.exports = Server;
