/*
 * ITSCAM Viewer - Frontend Application
 * Copyright (c) 2026 Pumatronix
 */

// State
let isConnected = false;
let isSubscribed = false;
let captureCount = 0;
let showOverlay = false;
let currentCapture = null;
const captureHistory = [];
const MAX_HISTORY = 20;

// DOM Elements
const elements = {
    // Connection
    connectionIndicator: document.getElementById('connectionIndicator'),
    cameraIP: document.getElementById('cameraIP'),
    cameraPort: document.getElementById('cameraPort'),
    password: document.getElementById('password'),
    connectBtn: document.getElementById('connectBtn'),
    disconnectBtn: document.getElementById('disconnectBtn'),

    // Device Info
    deviceSection: document.getElementById('deviceSection'),
    deviceVersion: document.getElementById('deviceVersion'),
    deviceSerial: document.getElementById('deviceSerial'),
    deviceProfiles: document.getElementById('deviceProfiles'),
    deviceActive: document.getElementById('deviceActive'),

    // Capture Control
    captureSection: document.getElementById('captureSection'),
    subscribeTrigger: document.getElementById('subscribeTrigger'),
    subscribeSnapshot: document.getElementById('subscribeSnapshot'),
    subscribeBtn: document.getElementById('subscribeBtn'),
    unsubscribeBtn: document.getElementById('unsubscribeBtn'),
    snapshotBtn: document.getElementById('snapshotBtn'),
    captureCount: document.getElementById('captureCount'),

    // Save Settings
    saveSection: document.getElementById('saveSection'),
    saveDirectory: document.getElementById('saveDirectory'),
    browseDirBtn: document.getElementById('browseDirBtn'),
    autoSave: document.getElementById('autoSave'),
    saveCurrentBtn: document.getElementById('saveCurrentBtn'),

    // Image Display
    captureImage: document.getElementById('captureImage'),
    noImage: document.getElementById('noImage'),
    imageInfo: document.getElementById('imageInfo'),
    imageSource: document.getElementById('imageSource'),
    imageFrame: document.getElementById('imageFrame'),
    imageSize: document.getElementById('imageSize'),
    imageTimestamp: document.getElementById('imageTimestamp'),
    showOverlay: document.getElementById('showOverlay'),
    metadataOverlay: document.getElementById('metadataOverlay'),
    
    // Overlay elements
    overlaySource: document.getElementById('overlaySource'),
    overlayFrame: document.getElementById('overlayFrame'),
    overlayTime: document.getElementById('overlayTime'),
    overlaySize: document.getElementById('overlaySize'),
    overlayExposure: document.getElementById('overlayExposure'),

    // Metadata Panel
    metadataSection: document.getElementById('metadataSection'),
    metaSource: document.getElementById('metaSource'),
    metaRequestId: document.getElementById('metaRequestId'),
    metaFrameCount: document.getElementById('metaFrameCount'),
    metaResolution: document.getElementById('metaResolution'),
    metaTimestamp: document.getElementById('metaTimestamp'),
    metaImageSize: document.getElementById('metaImageSize'),
    metaShutter: document.getElementById('metaShutter'),
    metaGain: document.getElementById('metaGain'),
    metaMultiExp: document.getElementById('metaMultiExp'),
    metaPlates: document.getElementById('metaPlates'),

    // History
    captureHistory: document.getElementById('captureHistory'),

    // Logs
    logContainer: document.getElementById('logContainer'),
    clearLogsBtn: document.getElementById('clearLogsBtn'),
};

// Initialize
document.addEventListener('DOMContentLoaded', () => {
    setupEventListeners();
    setupWailsEvents();
    addLog('info', 'Application ready');
});

function setupEventListeners() {
    elements.connectBtn.addEventListener('click', connect);
    elements.disconnectBtn.addEventListener('click', disconnect);
    elements.subscribeBtn.addEventListener('click', subscribe);
    elements.unsubscribeBtn.addEventListener('click', unsubscribe);
    elements.snapshotBtn.addEventListener('click', requestSnapshot);
    elements.clearLogsBtn.addEventListener('click', clearLogs);
    
    // Save settings
    elements.browseDirBtn.addEventListener('click', browseSaveDirectory);
    elements.autoSave.addEventListener('change', toggleAutoSave);
    elements.saveCurrentBtn.addEventListener('click', saveCurrentFrame);
    
    // Overlay toggle
    elements.showOverlay.addEventListener('change', toggleOverlay);

    // Enter key on inputs
    elements.cameraIP.addEventListener('keypress', (e) => {
        if (e.key === 'Enter') connect();
    });
    elements.password.addEventListener('keypress', (e) => {
        if (e.key === 'Enter') connect();
    });
}

function setupWailsEvents() {
    // Listen for capture events
    window.runtime.EventsOn('capture', (event) => {
        handleCapture(event);
    });

    // Listen for connection status changes
    window.runtime.EventsOn('connectionStatus', (status) => {
        updateConnectionStatus(status);
    });

    // Listen for log events
    window.runtime.EventsOn('log', (entry) => {
        addLog(entry.level.toLowerCase(), `[${entry.timestamp}] ${entry.message}`);
    });
}

// Connection Functions
async function connect() {
    const ip = elements.cameraIP.value.trim();
    const port = parseInt(elements.cameraPort.value);
    const password = elements.password.value;

    if (!ip) {
        addLog('error', 'Please enter a camera IP address');
        return;
    }

    elements.connectBtn.disabled = true;
    elements.connectBtn.textContent = 'Connecting...';
    updateConnectionIndicator('connecting', 'Connecting...');

    try {
        await window.go.main.App.Connect(ip, port, password);
        await loadDeviceInfo();
    } catch (err) {
        addLog('error', `Connection failed: ${err}`);
        updateConnectionIndicator('disconnected', 'Disconnected');
    } finally {
        elements.connectBtn.disabled = false;
        elements.connectBtn.textContent = 'Connect';
    }
}

async function disconnect() {
    try {
        await window.go.main.App.Disconnect();
    } catch (err) {
        addLog('error', `Disconnect failed: ${err}`);
    }
}

async function loadDeviceInfo() {
    try {
        const info = await window.go.main.App.GetDeviceInfo();
        if (info) {
            elements.deviceVersion.textContent = info.address || '-';
            elements.deviceSerial.textContent = info.port || '-';
            elements.deviceProfiles.textContent = '-';
            elements.deviceActive.textContent = info.activeProfile;
        }
    } catch (err) {
        addLog('error', `Failed to get device info: ${err}`);
    }
}

// Capture Functions
async function subscribe() {
    elements.subscribeBtn.disabled = true;

    const config = {
        trigger: elements.subscribeTrigger.checked,
        snapshot: elements.subscribeSnapshot.checked
    };

    if (!config.trigger && !config.snapshot) {
        addLog('warning', 'Please select at least one frame source');
        elements.subscribeBtn.disabled = false;
        return;
    }

    try {
        await window.go.main.App.SubscribeWithConfig(config);
        isSubscribed = true;
        elements.subscribeBtn.disabled = true;
        elements.unsubscribeBtn.disabled = false;
        addLog('info', `Subscribed to: ${config.trigger ? 'trigger' : ''} ${config.snapshot ? 'snapshot' : ''}`);
    } catch (err) {
        addLog('error', `Subscribe failed: ${err}`);
        elements.subscribeBtn.disabled = false;
    }
}

async function unsubscribe() {
    elements.unsubscribeBtn.disabled = true;

    try {
        await window.go.main.App.Unsubscribe();
        isSubscribed = false;
        elements.subscribeBtn.disabled = false;
        elements.unsubscribeBtn.disabled = true;
        addLog('info', 'Unsubscribed from capture events');
    } catch (err) {
        addLog('error', `Unsubscribe failed: ${err}`);
        elements.unsubscribeBtn.disabled = false;
    }
}

async function requestSnapshot() {
    elements.snapshotBtn.disabled = true;
    elements.snapshotBtn.textContent = 'Requesting...';

    try {
        await window.go.main.App.RequestSnapshot();
        addLog('info', 'Snapshot requested');
    } catch (err) {
        addLog('error', `Snapshot request failed: ${err}`);
    } finally {
        elements.snapshotBtn.disabled = false;
        elements.snapshotBtn.textContent = 'Request Snapshot';
    }
}

// Save Functions
async function browseSaveDirectory() {
    try {
        const dir = await window.go.main.App.SelectSaveDirectory();
        if (dir) {
            elements.saveDirectory.value = dir;
        }
    } catch (err) {
        addLog('error', `Failed to select directory: ${err}`);
    }
}

async function toggleAutoSave() {
    const enabled = elements.autoSave.checked;
    try {
        await window.go.main.App.SetAutoSave(enabled);
    } catch (err) {
        addLog('error', `Failed to set auto-save: ${err}`);
        elements.autoSave.checked = !enabled;
    }
}

async function saveCurrentFrame() {
    if (!currentCapture || !currentCapture.imageB64) {
        addLog('warning', 'No capture to save');
        return;
    }

    if (!elements.saveDirectory.value) {
        addLog('warning', 'Please select a save directory first');
        browseSaveDirectory();
        return;
    }

    elements.saveCurrentBtn.disabled = true;
    elements.saveCurrentBtn.textContent = 'Saving...';

    try {
        const result = await window.go.main.App.SaveFrameToFile(currentCapture.imageB64, '');
        if (result.success) {
            addLog('info', `Frame saved to: ${result.filePath}`);
        } else {
            addLog('error', `Save failed: ${result.error}`);
        }
    } catch (err) {
        addLog('error', `Save failed: ${err}`);
    } finally {
        elements.saveCurrentBtn.disabled = false;
        elements.saveCurrentBtn.textContent = 'Save Current Frame';
    }
}

// Overlay Functions
function toggleOverlay() {
    showOverlay = elements.showOverlay.checked;
    elements.metadataOverlay.style.display = showOverlay && currentCapture ? 'block' : 'none';
}

function updateOverlay(event) {
    if (!showOverlay) return;
    
    elements.overlaySource.textContent = event.source.toUpperCase();
    elements.overlayFrame.textContent = `#${event.frameCount}`;
    elements.overlayTime.textContent = event.timestamp;
    elements.overlaySize.textContent = `${event.width}x${event.height}`;
    elements.overlayExposure.textContent = `${event.shutter}µs / ${event.gain.toFixed(1)}dB`;
    elements.metadataOverlay.style.display = 'block';
}

// Event Handlers
function handleCapture(event) {
    captureCount++;
    elements.captureCount.textContent = captureCount;
    currentCapture = event;

    // Update main image
    const imageData = `data:image/jpeg;base64,${event.imageB64}`;
    elements.captureImage.src = imageData;
    elements.captureImage.style.display = 'block';
    elements.noImage.style.display = 'none';
    elements.imageInfo.style.display = 'flex';

    // Update image info bar
    const sourceLabel = event.source === 'trigger' ? 'TRG' : 'SNP';
    elements.imageSource.textContent = `Source: ${sourceLabel}`;
    elements.imageFrame.textContent = `Frame: ${event.frameCount} (Req: ${event.requestId})`;
    elements.imageSize.textContent = `${(event.imageSize / 1024).toFixed(1)} KB (${event.width}x${event.height})`;
    elements.imageTimestamp.textContent = event.timestamp;

    // Update metadata panel
    updateMetadataPanel(event);
    
    // Update overlay
    updateOverlay(event);

    // Add to history
    addToHistory(event.frameCount, imageData, event.timestamp, event.source);

    // Log plates if detected
    if (event.plates && event.plates.length > 0) {
        addLog('info', `Detected plates: ${event.plates.join(', ')}`);
    }
}

function updateMetadataPanel(event) {
    const sourceLabel = event.source === 'trigger' ? 'Trigger' : 'Snapshot';
    elements.metaSource.textContent = sourceLabel;
    elements.metaRequestId.textContent = event.requestId;
    elements.metaFrameCount.textContent = event.frameCount;
    elements.metaResolution.textContent = `${event.width} x ${event.height}`;
    elements.metaTimestamp.textContent = event.timestamp;
    elements.metaImageSize.textContent = `${(event.imageSize / 1024).toFixed(1)} KB`;
    elements.metaShutter.textContent = `${event.shutter} µs`;
    elements.metaGain.textContent = `${event.gain.toFixed(2)} dB`;
    
    if (event.multiExpLength > 1) {
        elements.metaMultiExp.textContent = `${event.multiExpIndex + 1} / ${event.multiExpLength}`;
    } else {
        elements.metaMultiExp.textContent = 'Single';
    }
    
    if (event.plates && event.plates.length > 0) {
        elements.metaPlates.textContent = event.plates.join(', ');
    } else {
        elements.metaPlates.textContent = 'None';
    }
}

function addToHistory(frameId, imageData, timestamp, source) {
    // Remove empty hint
    const emptyHint = elements.captureHistory.querySelector('.empty-hint');
    if (emptyHint) {
        emptyHint.remove();
    }

    // Create history item
    const item = document.createElement('div');
    item.className = 'history-item';
    const sourceClass = source === 'trigger' ? 'source-trigger' : 'source-snapshot';
    item.innerHTML = `
        <img src="${imageData}" alt="Capture ${frameId}">
        <div class="info">
            <span class="source-badge ${sourceClass}">${source === 'trigger' ? 'TRG' : 'SNP'}</span>
            #${frameId}
        </div>
    `;
    item.addEventListener('click', () => {
        elements.captureImage.src = imageData;
    });

    // Add to beginning
    elements.captureHistory.insertBefore(item, elements.captureHistory.firstChild);

    // Limit history
    captureHistory.unshift({ frameId, imageData, timestamp, source });
    while (captureHistory.length > MAX_HISTORY) {
        captureHistory.pop();
        const lastItem = elements.captureHistory.lastChild;
        if (lastItem && lastItem.classList.contains('history-item')) {
            lastItem.remove();
        }
    }
}

// UI Updates
function updateConnectionStatus(status) {
    isConnected = status.connected;

    if (status.connected) {
        updateConnectionIndicator('connected', status.state);
        elements.connectBtn.disabled = true;
        elements.disconnectBtn.disabled = false;
        elements.deviceSection.style.display = 'block';
        elements.captureSection.style.display = 'block';
        elements.saveSection.style.display = 'block';
    } else {
        updateConnectionIndicator('disconnected', status.state);
        elements.connectBtn.disabled = false;
        elements.disconnectBtn.disabled = true;
        elements.deviceSection.style.display = 'none';
        elements.captureSection.style.display = 'none';
        elements.saveSection.style.display = 'none';

        // Reset subscription state
        isSubscribed = false;
        elements.subscribeBtn.disabled = false;
        elements.unsubscribeBtn.disabled = true;
    }

    if (status.message) {
        addLog('info', `Connection: ${status.state} - ${status.message}`);
    }
}

function updateConnectionIndicator(state, text) {
    elements.connectionIndicator.className = `connection-indicator ${state}`;
    elements.connectionIndicator.querySelector('.status-text').textContent = text;
}

// Logging
function addLog(level, message) {
    const entry = document.createElement('p');
    entry.className = `log-entry ${level}`;
    entry.innerHTML = message;

    elements.logContainer.appendChild(entry);
    elements.logContainer.scrollTop = elements.logContainer.scrollHeight;

    // Limit log entries
    while (elements.logContainer.children.length > 200) {
        elements.logContainer.removeChild(elements.logContainer.firstChild);
    }
}

function clearLogs() {
    elements.logContainer.innerHTML = '';
    addLog('info', 'Logs cleared');
}
