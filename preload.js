const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('electronAPI', {
  getInitialFile: () => ipcRenderer.invoke('get-initial-file'),
  onFileOpened: (callback) => ipcRenderer.on('file-opened', (_, data) => callback(data))
});
