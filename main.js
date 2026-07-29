const { app, BrowserWindow, Menu, ipcMain } = require('electron');
const path = require('path');
const fs = require('fs');

let mainWindow;
let pendingFilePath = null;

function getAppIcon() {
  const iconFile = process.platform === 'linux' ? 'icon.png' : 'icon.ico';
  return path.join(__dirname, iconFile);
}

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1200,
    height: 800,
    minWidth: 800,
    minHeight: 500,
    title: 'JSON 阅读器',
    icon: getAppIcon(),
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, 'preload.js')
    }
  });

  mainWindow.loadFile('index.html');

  // 去掉默认菜单栏
  Menu.setApplicationMenu(null);

  mainWindow.on('closed', () => {
    mainWindow = null;
  });

  // 页面加载完成后，如果有待处理的文件路径则发送
  mainWindow.webContents.on('did-finish-load', () => {
    if (pendingFilePath) {
      sendFileToRenderer(pendingFilePath);
      pendingFilePath = null;
    }
  });
}

// 发送文件到渲染进程
function sendFileToRenderer(filePath) {
  if (!mainWindow) return;
  try {
    const content = fs.readFileSync(filePath, 'utf-8');
    const fileName = path.basename(filePath);
    mainWindow.webContents.send('file-opened', { name: fileName, content });
  } catch (e) {
    console.error('Failed to read file:', e.message);
  }
}

// IPC: 渲染进程请求初始文件
ipcMain.handle('get-initial-file', async () => {
  if (!pendingFilePath) return null;
  try {
    const content = fs.readFileSync(pendingFilePath, 'utf-8');
    const fileName = path.basename(pendingFilePath);
    const fp = pendingFilePath;
    pendingFilePath = null;
    return { name: fileName, content };
  } catch (e) {
    pendingFilePath = null;
    return null;
  }
});

// 处理命令行参数中的文件路径
function parseFileFromArgv(argv) {
  for (let i = 1; i < argv.length; i++) {
    const arg = argv[i];
    // 跳过 Electron 自身的参数
    if (arg.startsWith('--') || arg.startsWith('-')) continue;
    // 跳过 app 自身路径
    if (arg.endsWith('.exe') || arg.includes('electron')) continue;
    if (fs.existsSync(arg)) {
      const ext = path.extname(arg).toLowerCase();
      if (['.json', '.har', '.txt', '.js', '.jsonc'].includes(ext)) {
        return arg;
      }
    }
  }
  return null;
}

app.whenReady().then(() => {
  // 检查命令行参数
  const filePath = parseFileFromArgv(process.argv);
  if (filePath) {
    pendingFilePath = filePath;
  }
  createWindow();
});

// macOS: open-file 事件
app.on('open-file', (event, filePath) => {
  event.preventDefault();
  if (mainWindow && mainWindow.webContents) {
    sendFileToRenderer(filePath);
  } else {
    pendingFilePath = filePath;
  }
});

// Windows: second-instance 事件（单实例模式）
const gotTheLock = app.requestSingleInstanceLock();
if (!gotTheLock) {
  app.quit();
} else {
  app.on('second-instance', (event, argv) => {
    // 有人试图启动第二个实例，把文件路径发给已有窗口
    if (mainWindow) {
      if (mainWindow.isMinimized()) mainWindow.restore();
      mainWindow.focus();
      const filePath = parseFileFromArgv(argv);
      if (filePath) {
        sendFileToRenderer(filePath);
      }
    }
  });
}

app.on('window-all-closed', () => {
  app.quit();
});

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow();
  }
});
