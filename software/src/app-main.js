'use strict';
const {app, BrowserWindow, ipcMain, shell, dialog, Menu} = require('electron');

const path = require('path');
const url = require('url');
const fs = require('fs');
const appMenu = require('./app-menu');
const appServer = require('./app-server');

let mainWindow, serve;
const args = process.argv.slice(1);
serve = args.some(val => val === "--serve");

if (serve) {
  require('electron-reload')(__dirname, {
    electron: require('${__dirname}/../../node_modules/electron')
  });
  require('electron-context-menu')({
  	prepend: (params, browserWindow) => [{
  	}]
  });
}

function createWindow() {

  Menu.setApplicationMenu(Menu.buildFromTemplate(appMenu));

  let width = 1200, height = 860;

  const screen = require('electron').screen;
  if (screen) {
    const display = screen.getPrimaryDisplay();
    if (display && display.workArea) {
      width = display.workArea.width || width;
      height = display.workArea.height || height;
    }
  }

  mainWindow = new BrowserWindow({
    width: width,
    height: height,
    minWidth: 400,
    minHeight: 400,
    textAreasAreResizable: false,
    plugins: true,
    show: false,
    icon: __dirname + '/favicon.ico'
  });

  //mainWindow.loadURL('http://localhost:9061');

  mainWindow.loadURL(url.format({
    pathname: path.join(__dirname, 'index.html'),
    protocol: 'file:',
    slashes: true
  }))

  if (serve) {
    mainWindow.webContents.openDevTools();
  }

  mainWindow.once('ready-to-show', () => {
    mainWindow.show();
    mainWindow.maximize();
  });

  //mainWindow.setProgressBar(0.5, {mode: 'normal'});

  mainWindow.webContents.on('did-finish-load', () => {
    /*
    mainWindow.webContents.printToPDF({}, (error, data) => {
      if (error) throw error;
      fs.writeFile(path.join(__dirname, '/print.pdf'), data, (error) => {
        if (error) throw error;
        console.log('Write PDF successfully.');
      });
    });
    */
    let WiFiControl = require('wifi-control');

    WiFiControl.init({
      debug: true,
      connectionTimeout: 5000
    });

    const WebSocket = require('ws');
    const express = require('express');
    const path = require('path');
    const app = express();
    const server = require('http').createServer();

    mainWindow.webContents.send('request', 'ipcStart');
    ipcMain.on('response', (event, arg) => {
      if (arg['reason'] === 'checkWifi') {
        let ifaceState = WiFiControl.getIfaceState();
        let connected = false;
        if (ifaceState['ssid'] === 'SimcirHW') {
          //appServer.startServer();

          app.use(express.static(path.join(__dirname, '/')));

          const wss = new WebSocket.Server({server: server});

          wss.on('connection', (ws) => {
            mainWindow.webContents.send('apReady', 'ok');
            ws.on('message', (data) => {
              wss.clients.forEach((client) => {
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

          connected = true;
        }
        event.sender.send('checkWifi', {connected: connected});

      } else if (arg['reason'] === 'saveFile') {
        dialog.showSaveDialog(mainWindow,
          {
            title: 'Salvar como...',
            buttonLabel: 'Salvar',
            filters: [
              {name: 'SimcirHW', extensions: ['json', 'simcirhw']}
            ]
          },
          (filename) => {
            if (typeof filename !== 'undefined') {
              fs.writeFileSync(filename, arg['content'], 'utf-8');
            }
          }
        );
      } else if (arg['reason'] === 'openFile') {
        dialog.showOpenDialog(mainWindow,
          {
            title: 'Abrir...',
            buttonLabel: 'Abrir',
            filters: [
              {name: 'SimcirHW', extensions: ['json', 'simcirhw']}
            ]
          },
          (filename) => {
            if (typeof filename !== 'undefined') {
              console.log(filename);
              event.sender.send('readFile', {content: fs.readFileSync(filename[0], 'utf-8')});
            }
          }
        );
      }
    });
  });

  mainWindow.on('closed', function () {
    mainWindow = null;
  })

}

app.on('ready', createWindow);

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('activate', () => {
  if (mainWindow === null) {
    createWindow();
  }
});
