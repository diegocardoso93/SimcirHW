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

  appServer.startServer();
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

  /*
  dialog.showSaveDialog(mainWindow,
    {
      title: 'Salvar como...',
      buttonLabel: 'Salvar',
      filters: [
        {name: 'SimcirHW Schematic File', extensions: ['simcirhw']}
      ]
    },
    (filename) => console.log(filename)
  );*/

  mainWindow.loadURL('http://localhost:9061');

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
