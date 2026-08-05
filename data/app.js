// Navigation
function activateNavLink(target) {
    document.querySelectorAll('.sidebar-nav a').forEach(link => {
        if (link.getAttribute('data-target') === target) {
            link.classList.add('active');
        } else {
            link.classList.remove('active');
        }
    });
}

function activateView(target) {
    document.querySelectorAll('.view').forEach(v => v.classList.remove('active'));
    const view = document.getElementById(`view-${target}`);
    if (view) {
        view.classList.add('active');
    }
}

// Sidebar Drawer Logic for mobile
function toggleSidebar() {
    document.getElementById('sidebar').classList.toggle('open');
    document.getElementById('sidebar-overlay').classList.toggle('active');
}

function closeSidebar() {
    document.getElementById('sidebar').classList.remove('open');
    document.getElementById('sidebar-overlay').classList.remove('active');
}

document.getElementById('menu-toggle').addEventListener('click', toggleSidebar);
document.getElementById('sidebar-overlay').addEventListener('click', closeSidebar);

document.querySelectorAll('.sidebar-nav a').forEach(link => {
    link.addEventListener('click', (e) => {
        const target = link.getAttribute('data-target');
        if (!target) {
            return;
        }
        e.preventDefault();

        history.pushState(null, '', target === 'dashboard' ? '/' : `/${target}`);
        document.getElementById('page-title').innerText = link.innerText;
        activateNavLink(target);
        activateView(target);
        loadView(target);
        closeSidebar();
    });
});

// Theme Toggle
function setupThemeToggle() {
    const btn = document.getElementById('theme-toggle');
    btn.addEventListener('click', () => {
        if(document.body.classList.contains('dark-mode')) {
            document.body.classList.replace('dark-mode', 'light-mode');
            btn.innerText = '🌙 Dark Mode';
            localStorage.setItem('theme', 'light');
            Chart.defaults.color = '#0f172a';
        } else {
            document.body.classList.replace('light-mode', 'dark-mode');
            btn.innerText = '☀️ Light Mode';
            localStorage.setItem('theme', 'dark');
            Chart.defaults.color = '#f8fafc';
        }
        renderCharts();
    });
}
Chart.defaults.color = '#f8fafc'; 

let currentPage = 1;
let itemsPerPage = 15;
let use24hFormat = true;

setInterval(() => {
    const d = new Date();
    document.getElementById('live-clock').innerText = d.toLocaleTimeString([], {hour12: !use24hFormat});
}, 1000);

function formatDuration(seconds) {
    if (seconds == 0) return "0 Seconds";
    let h = Math.floor(seconds / 3600);
    let m = Math.floor((seconds % 3600) / 60);
    let s = seconds % 60;
    
    let res = "";
    if (h > 0) res += h + " Hour" + (h > 1 ? "s " : " ");
    if (m > 0) res += m + " Minute" + (m > 1 ? "s " : " ");
    if (s > 0 || (h == 0 && m == 0)) res += s + " Second" + (s != 1 ? "s" : "");
    return res.trim();
}

function formatDate(epoch) {
    const d = new Date(epoch * 1000);
    return d.toISOString().split('T')[0];
}

function formatTime(epoch) {
    const d = new Date(epoch * 1000);
    return d.toLocaleTimeString([], {hour12: !use24hFormat});
}

const eventTypes = ["🟢 Available", "🔴 Lost", "🔵 Restart", "⚪ System"];
const eventClasses = ["type-0", "type-1", "type-2", "type-3"];

function getEventTypeName(type) {
    return eventTypes[type] || "Unknown";
}

function loadView(view) {
    if (view === 'dashboard' || view === 'home') return loadHome();
    if (view === 'history') return loadHistory();
    if (view === 'daily') return loadDaily();
    if (view === 'weekly') return loadWeekly();
    if (view === 'monthly') return loadMonthly();
    if (view === 'statistics') return loadStatistics();
    if (view === 'settings') return loadSettings();
}

function currentRoute() {
    if (window.location.hash) {
        return window.location.hash.substring(1) || 'dashboard';
    }
    const path = window.location.pathname.replace(/^\//, '').replace(/\/$/, '');
    return path === '' ? 'dashboard' : path;
}

function normalizeRoute(route) {
    const validRoutes = ['dashboard', 'history', 'daily', 'weekly', 'monthly', 'statistics', 'settings'];
    return validRoutes.includes(route) ? route : 'dashboard';
}

function navigateTo(route) {
    history.replaceState(null, '', route === 'dashboard' ? '/' : `/${route}`);
    document.getElementById('page-title').innerText = route === 'dashboard' ? 'Dashboard' : route.charAt(0).toUpperCase() + route.slice(1);
    activateNavLink(route);
    activateView(route);
    loadView(route);
}

async function fetchJSON(url) {
    try {
        const res = await fetch(url, { credentials: 'include' });
        return await res.json();
    } catch (e) {
        console.error("Fetch error:", e);
        return null;
    }
}

async function loadHome() {
    const stats = await fetchJSON('/api/statistics');
    if(stats) {
        document.getElementById('home-session').innerText = formatDuration(stats.currentSession);
        document.getElementById('home-total-outages').innerText = stats.totalOutagesCount;
        document.getElementById('home-uptime').innerText = formatDuration(stats.currentSession);
        document.getElementById('home-last-outage').innerText = stats.lastOutage > 0 ? new Date(stats.lastOutage * 1000).toLocaleString() : 'None';
    }
    
    const events = await fetchJSON('/api/events?page=1&limit=5');
    if(events && events.length > 0) {
        const latest = events[0];
        const banner = document.getElementById('current-status-banner');
        const txt = document.getElementById('status-text');
        
        banner.className = 'status-banner glass';
        if(latest.type === 1) { // Lost
            banner.classList.add('status-off');
            txt.innerText = 'Power is currently LOST since ' + formatTime(latest.timestamp);
        } else {
            banner.classList.add('status-on');
            txt.innerText = 'Electricity is AVAILABLE';
        }
    } else {
        const banner = document.getElementById('current-status-banner');
        const txt = document.getElementById('status-text');
        banner.className = 'status-banner glass status-on';
        txt.innerText = 'Electricity is AVAILABLE (No Events)';
    }
    
    loadDailyChart('homeTimelineChart');
}

async function loadHistory(page = 1) {
    try {
        const res = await fetch(`/api/events?page=${page}&limit=${itemsPerPage}`, { credentials: 'include' });
        const data = await res.json();
        
        const tbody = document.getElementById('history-tbody');
        tbody.innerHTML = '';
        
        if (data.length === 0) {
            tbody.innerHTML = '<tr><td colspan="5">No events logged yet.</td></tr>';
            return;
        }
        
        data.forEach(e => {
            const date = new Date(e.timestamp * 1000);
            const tr = document.createElement('tr');
            tr.innerHTML = `
                <td>${date.toLocaleDateString()}</td>
                <td>${date.toLocaleTimeString([], {hour12: !use24hFormat})}</td>
                <td class="type-${e.type}">${getEventTypeName(e.type)}</td>
                <td>${formatDuration(e.duration)}</td>
                <td>${e.desc}</td>
            `;
            tbody.appendChild(tr);
        });
        
        document.getElementById('page-num').innerText = page;
        
    } catch (err) {
        console.error(err);
    }
}

document.getElementById('btn-export-csv').addEventListener('click', async () => {
    try {
        const res = await fetch('/api/events?page=1&limit=10000', { credentials: 'include' });
        const data = await res.json();
        let csvContent = "data:text/csv;charset=utf-8,Date,Time,Event Type,Duration,Description\n";
        data.forEach(e => {
            const date = new Date(e.timestamp * 1000);
            csvContent += `${date.toLocaleDateString()},${date.toLocaleTimeString([], {hour12: !use24hFormat})},${getEventTypeName(e.type)},${e.duration},"${e.desc}"\n`;
        });
        const encodedUri = encodeURI(csvContent);
        const link = document.createElement("a");
        link.setAttribute("href", encodedUri);
        link.setAttribute("download", "power_outage_events.csv");
        document.body.appendChild(link);
        link.click();
        link.remove();
    } catch (e) { alert("Export failed"); }
});

document.getElementById('btn-export-json').addEventListener('click', async () => {
    try {
        const res = await fetch('/api/events?page=1&limit=10000', { credentials: 'include' });
        const data = await res.json();
        const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(data, null, 2));
        const link = document.createElement("a");
        link.setAttribute("href", dataStr);
        link.setAttribute("download", "power_outage_events.json");
        document.body.appendChild(link);
        link.click();
        link.remove();
    } catch (e) { alert("Export failed"); }
});

function prevPage() {
    if(currentPage > 1) {
        currentPage--;
        loadHistory(currentPage);
    }
}

function nextPage() {
    currentPage++;
    loadHistory(currentPage);
}

async function clearHistory() {
    if(confirm("Are you sure you want to clear all history?")) {
        await fetch('/api/clear-history', { method: 'POST', credentials: 'include' });
        currentPage = 1;
        loadHistory(currentPage);
    }
}

let charts = {};

function initChart(canvasId, type, data, options) {
    if(charts[canvasId]) {
        charts[canvasId].destroy();
    }
    const ctx = document.getElementById(canvasId).getContext('2d');
    charts[canvasId] = new Chart(ctx, { type, data, options });
}

async function loadDailyChart(canvasId = 'dailyChart') {
    const data = await fetchJSON('/api/daily');
    if(!data) return;
    
    const labels = data.map(d => d.date);
    const durations = data.map(d => d.outageDuration / 60); 
    
    initChart(canvasId, 'bar', {
        labels,
        datasets: [{
            label: 'Outage Duration (Minutes)',
            data: durations,
            backgroundColor: 'rgba(239, 68, 68, 0.6)',
            borderColor: '#ef4444',
            borderWidth: 1,
            borderRadius: 4
        }]
    }, { responsive: true, maintainAspectRatio: false });
}

async function loadDaily() {
    await loadDailyChart('dailyChart');
}

async function loadWeekly() {
    const data = await fetchJSON('/api/weekly');
    if(!data) return;
    
    const labels = data.map(d => d.week);
    const durations = data.map(d => d.outageDuration / 60);
    
    initChart('weeklyChart', 'bar', {
        labels,
        datasets: [{
            label: 'Outage Duration (Minutes)',
            data: durations,
            backgroundColor: 'rgba(245, 158, 11, 0.6)',
            borderColor: '#f59e0b',
            borderWidth: 1,
            borderRadius: 4
        }]
    }, { responsive: true, maintainAspectRatio: false });
}

async function loadMonthly() {
    const data = await fetchJSON('/api/monthly');
    if(!data) return;
    
    const labels = data.map(d => d.month);
    const durations = data.map(d => d.outageDuration / 60);
    
    initChart('monthlyChart', 'bar', {
        labels,
        datasets: [{
            label: 'Outage Duration (Minutes)',
            data: durations,
            backgroundColor: 'rgba(59, 130, 246, 0.6)',
            borderColor: '#3b82f6',
            borderWidth: 1,
            borderRadius: 4
        }]
    }, { responsive: true, maintainAspectRatio: false });
}

function renderCharts() {
    const activeView = document.querySelector('.view.active').id;
    if(activeView === 'view-dashboard') loadHome();
    if(activeView === 'view-daily') loadDaily();
    if(activeView === 'view-weekly') loadWeekly();
    if(activeView === 'view-monthly') loadMonthly();
}

async function loadStatistics() {
    const stats = await fetchJSON('/api/statistics');
    if(!stats) return;
    
    document.getElementById('stat-total-outage').innerText = formatDuration(stats.totalOutage);
    document.getElementById('stat-longest-outage').innerText = formatDuration(stats.longestOutage);
    document.getElementById('stat-shortest-outage').innerText = formatDuration(stats.shortestOutage);
    document.getElementById('stat-average-outage').innerText = formatDuration(stats.averageOutage);
}

async function loadSettings() {
    const s = await fetchJSON('/api/settings');
    if(!s) return;
    
    document.getElementById('set-name').value = s.deviceName || '';
    document.getElementById('set-ssid').value = s.wifiSSID || '';
    document.getElementById('set-ap-ssid').value = s.apSSID || 'EspElectrycityMonitor';
    document.getElementById('set-ntp').value = s.ntpServer || 'pool.ntp.org';
    document.getElementById('set-tz').value = s.timezoneOffset || 0;
    
    use24hFormat = s.use24hFormat === undefined ? true : s.use24hFormat;
    document.getElementById('set-timeformat').value = use24hFormat ? "24" : "12";
    
    document.getElementById('set-tg-enabled').checked = s.telegramEnabled || false;
    document.getElementById('set-tg-token').value = s.telegramBotToken || '';
    document.getElementById('set-tg-chat').value = s.telegramChatId || '';
    document.getElementById('set-buzzer-enabled').checked = s.buzzerEnabled || false;
    document.getElementById('set-buzzer-pin').value = s.buzzerPin || -1;
    
    // Check WiFi connection state and toggle UI
    updateWifiUI();
}

async function updateWifiUI() {
    try {
        const wifiStatus = await fetchJSON('/api/wifi-status');
        const connectedDiv = document.getElementById('wifi-connected');
        const setupDiv = document.getElementById('wifi-setup');
        
        if (wifiStatus && wifiStatus.status === 'connected') {
            document.getElementById('wifi-connected-ssid').innerText = wifiStatus.ssid || '??';
            document.getElementById('wifi-connected-ip').innerText = wifiStatus.ip || '--';
            document.getElementById('wifi-connected-rssi').innerText = wifiStatus.rssi || '--';
            connectedDiv.style.display = 'block';
            setupDiv.style.display = 'none';
        } else {
            connectedDiv.style.display = 'none';
            setupDiv.style.display = 'block';
        }
    } catch(e) {
        console.error('WiFi status check failed', e);
    }
}

async function scanWifiNetworks() {
    const btn = document.getElementById('scan-wifi-btn');
    btn.disabled = true;
    btn.innerText = 'Scanning...';
    try {
        let networks = null;
        for (let i = 0; i < 15; i++) {
            const res = await fetch('/api/wifi-scan');
            if (res.status === 202) {
                await new Promise(r => setTimeout(r, 1000));
            } else if (res.status === 200) {
                networks = await res.json();
                break;
            } else {
                throw new Error("Scan failed");
            }
        }
        if (!networks) throw new Error("Timeout scanning");

        const select = document.getElementById('wifi-networks');
        select.innerHTML = '<option value="">Select a network</option>';
        if (networks && networks.length > 0) {
            networks.sort((a, b) => b.rssi - a.rssi);
            networks.forEach(net => {
                const opt = document.createElement('option');
                opt.value = net.ssid;
                opt.text = `${net.ssid} (${net.secure ? 'Secure' : 'Open'}) ${net.rssi}dBm`;
                select.appendChild(opt);
            });
        } else {
            select.innerHTML = '<option value="">No networks found</option>';
        }
    } catch (e) {
        alert('Failed to scan WiFi networks');
    } finally {
        btn.disabled = false;
        btn.innerText = 'Scan Nearby Networks';
    }
}

async function connectWifiNow() {
    const ssid = document.getElementById('set-ssid').value.trim();
    const password = document.getElementById('set-pass').value;
    if (!ssid) {
        alert('Please select or enter a WiFi SSID first.');
        return;
    }

    const btn = document.getElementById('connect-wifi-btn');
    btn.disabled = true;
    btn.innerText = 'Connecting...';

    try {
        const res = await fetch('/api/wifi-connect', {
            method: 'POST',
            credentials: 'include',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ ssid, password })
        });
        const result = await res.json();
        
        if (res.ok) {
            let connected = false;
            for (let i = 0; i < 20; i++) {
                await new Promise(r => setTimeout(r, 1000));
                try {
                    const statusRes = await fetch('/api/wifi-status');
                    const statusResult = await statusRes.json();
                    if (statusResult.status === 'connected') {
                        connected = true;
                        break;
                    } else if (statusResult.status === 'failed') {
                        break;
                    }
                } catch(e) {
                    // Ignore fetch errors during polling as IP might change or network drops
                }
            }
            if (connected) {
                alert('Connected to WiFi network successfully.');
                updateWifiUI();
            } else {
                alert('WiFi connection failed or timed out. Please check SSID and password.');
            }
        } else {
            alert(result.message || 'WiFi connection request failed.');
        }
    } catch (e) {
        alert('Unable to connect to WiFi.');
    } finally {
        btn.disabled = false;
        btn.innerText = 'Connect';
    }
}

document.getElementById('scan-wifi-btn').addEventListener('click', scanWifiNetworks);
document.getElementById('wifi-networks').addEventListener('change', (e) => {
    const ssid = e.target.value;
    if (ssid) {
        document.getElementById('set-ssid').value = ssid;
    }
});
document.getElementById('connect-wifi-btn').addEventListener('click', connectWifiNow);

document.getElementById('forget-wifi-btn').addEventListener('click', async () => {
    if (!confirm('Are you sure you want to forget this WiFi network? The device will disconnect from the router.')) return;
    try {
        const res = await fetch('/api/wifi-forget', { method: 'POST', credentials: 'include' });
        if (res.ok) {
            alert('WiFi network forgotten. You can now connect to a new network.');
            document.getElementById('set-ssid').value = '';
            document.getElementById('set-pass').value = '';
            updateWifiUI();
        } else {
            alert('Failed to forget WiFi network.');
        }
    } catch(e) {
        alert('Network error while forgetting WiFi.');
    }
});

document.getElementById('settings-form').addEventListener('submit', async (e) => {    e.preventDefault();
    const data = {
        deviceName: document.getElementById('set-name').value,
        wifiSSID: document.getElementById('set-ssid').value,
        apSSID: document.getElementById('set-ap-ssid').value,
        ntpServer: document.getElementById('set-ntp').value,
        timezoneOffset: parseInt(document.getElementById('set-tz').value),
        telegramEnabled: document.getElementById('set-tg-enabled').checked,
        telegramBotToken: document.getElementById('set-tg-token').value,
        telegramChatId: document.getElementById('set-tg-chat').value,
        buzzerEnabled: document.getElementById('set-buzzer-enabled').checked,
        buzzerPin: parseInt(document.getElementById('set-buzzer-pin').value),
        use24hFormat: document.getElementById('set-timeformat').value === "24"
    };
    
    const pass = document.getElementById('set-pass').value;
    if(pass) data.wifiPassword = pass;

    const apPass = document.getElementById('set-ap-pass').value;
    if(apPass) data.apPassword = apPass;
    
    const adminPass = document.getElementById('set-admin-pass').value;
    if(adminPass) data.adminPassword = adminPass;
    
    try {
        const res = await fetch('/api/settings', {
            method: 'POST',
            credentials: 'include',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify(data)
        });
        
        const result = await res.json();
        
        if (res.ok) {
            alert('Settings saved!');
            document.getElementById('set-pass').value = '';
            document.getElementById('set-ap-pass').value = '';
            document.getElementById('set-admin-pass').value = '';
            if (adminPass) {
                alert('Admin password changed! You may be prompted to log in again.');
                window.location.reload();
            }
        } else {
            alert('Error: ' + (result.message || 'Failed to save settings'));
        }
    } catch (e) {
        alert('Network error saving settings');
    }
});

async function restartDevice() {
    if(confirm("Restart the ESP8266?")) {
        await fetch('/api/restart', { method: 'POST', credentials: 'include' });
        setTimeout(() => location.reload(), 3000);
    }
}

window.addEventListener('popstate', () => {
    const route = normalizeRoute(currentRoute());
    document.getElementById('page-title').innerText = route === 'dashboard' ? 'Dashboard' : route.charAt(0).toUpperCase() + route.slice(1);
    activateNavLink(route);
    activateView(route);
    loadView(route);
});

window.onload = () => {
    const route = normalizeRoute(currentRoute());
    document.getElementById('page-title').innerText = route === 'dashboard' ? 'Dashboard' : route.charAt(0).toUpperCase() + route.slice(1);
    activateNavLink(route);
    activateView(route);
    loadView(route);
};
