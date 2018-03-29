
const {app, Menu} = require('electron')

const template = [
  {
    label: 'Arquivo',
    submenu: [
      {role: 'quit', label: 'Sair'}
    ]
  },
  {
    label: 'Editar',
    submenu: [
      {role: 'undo', label: 'Desfazer'},
      {role: 'redo', label: 'Refazer'},
      {type: 'separator'},
      {role: 'cut', label: 'Recortar'},
      {role: 'copy', label: 'Copiar'},
      {role: 'paste', label: 'Colar'},
      //{role: 'pasteandmatchstyle'},
      {role: 'delete', label: 'Excluir'},
      {role: 'selectall', label: 'Selecionar tudo'}
    ]
  },
  {
    label: 'Visualizar',
    submenu: [
      {role: 'reload', label: 'Recarregar'},
      {role: 'forcereload', label: 'Forçar Recarregar'},
      {role: 'toggledevtools', label: 'Dev Tools'},
      {type: 'separator'},
      {role: 'resetzoom', label: 'Limpar Zoom'},
      {role: 'zoomin', label: 'Aumentar Zoom'},
      {role: 'zoomout', label: 'Diminuir Zoom'},
      {type: 'separator'},
      {role: 'togglefullscreen', label: 'Tela Cheia'}
    ]
  },
  {
    role: 'window',
	label: 'Janela',
    submenu: [
      {role: 'minimize', label: 'Minimizar'},
      {role: 'close', label: 'Fechar'}
    ]
  },
  {
    role: 'help',
	label: 'Ajuda',
	submenu: [
      {
        label: 'Guia de Instruções',
        click () { require('electron').shell.openExternal('http://simcirhw.readthedocs.io') }
      }
    ]
  }
]

module.exports = template;