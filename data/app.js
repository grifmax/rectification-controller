// Smart-Column S3 - Web UI JavaScript

let ws = null;
let reconnectInterval = null;
let isConnected = false;
let miniChart = null;
let miniChartData = {
    timestamps: [],
    cube: [],
    columnTop: [],
    reflux: []
};
const MINI_CHART_MAX_POINTS = 60; // 5 минут при обновлении каждые 5 секунд

// Инициализация при загрузке страницы
document.addEventListener('DOMContentLoaded', function() {
    initTabs();
    loadTheme();
    initMiniChart();
    connectWebSocket();
});

// ============================================================================
// WebSocket
// ============================================================================

function connectWebSocket() {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${window.location.hostname}/ws`;

    addLog('Подключение к WebSocket...');

    try {
        ws = new WebSocket(wsUrl);

        ws.onopen = function() {
            isConnected = true;
            updateConnectionStatus(true);
            addLog('✅ Подключено к контроллеру', 'info');

            // Остановить попытки переподключения
            if (reconnectInterval) {
                clearInterval(reconnectInterval);
                reconnectInterval = null;
            }
        };

        ws.onmessage = function(event) {
            try {
                const data = JSON.parse(event.data);
                updateUI(data);
            } catch (e) {
                console.error('Ошибка парсинга JSON:', e);
            }
        };

        ws.onerror = function(error) {
            console.error('WebSocket error:', error);
            addLog('❌ Ошибка подключения', 'error');
        };

        ws.onclose = function() {
            isConnected = false;
            updateConnectionStatus(false);
            addLog('⚠️ Соединение разорвано. Переподключение...', 'warning');

            // Попытка переподключения каждые 5 секунд
            if (!reconnectInterval) {
                reconnectInterval = setInterval(() => {
                    if (!isConnected) {
                        connectWebSocket();
                    }
                }, 5000);
            }
        };
    } catch (e) {
        console.error('Ошибка создания WebSocket:', e);
        updateConnectionStatus(false);
    }
}

function sendCommand(action, param = '', value = 0) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        const cmd = { action, param, value };
        ws.send(JSON.stringify(cmd));
        addLog(`📤 Команда: ${action} ${param} ${value}`);
    } else {
        addLog('❌ Нет подключения к контроллеру', 'error');
    }
}

function updateConnectionStatus(connected) {
    const statusDot = document.getElementById('connection-status');
    const statusText = document.getElementById('connection-text');

    if (connected) {
        statusDot.className = 'status-dot online';
        statusText.textContent = 'Подключено';
    } else {
        statusDot.className = 'status-dot offline';
        statusText.textContent = 'Отключено';
    }
}

// ============================================================================
// Mini Chart
// ============================================================================

function initMiniChart() {
    const options = {
        chart: {
            type: 'line',
            height: 200,
            animations: {
                enabled: true,
                dynamicAnimation: {
                    speed: 500
                }
            },
            toolbar: {
                show: false
            },
            background: 'transparent'
        },
        theme: {
            mode: document.body.getAttribute('data-theme') || 'light'
        },
        series: [
            {
                name: 'Куб',
                data: []
            },
            {
                name: 'Царга верх',
                data: []
            },
            {
                name: 'Дефлегматор',
                data: []
            }
        ],
        xaxis: {
            type: 'datetime',
            labels: {
                datetimeFormatter: {
                    minute: 'HH:mm'
                }
            }
        },
        yaxis: {
            title: {
                text: '°C'
            },
            decimalsInFloat: 1
        },
        stroke: {
            curve: 'smooth',
            width: 2
        },
        colors: ['#dc3545', '#007bff', '#17a2b8'],
        legend: {
            show: true,
            position: 'top'
        },
        tooltip: {
            x: {
                format: 'HH:mm:ss'
            }
        }
    };

    miniChart = new ApexCharts(document.querySelector("#mini-chart"), options);
    miniChart.render();
}

function updateMiniChart(data) {
    if (!miniChart) return;

    const now = new Date().getTime();

    // Добавить новые данные
    if (data.t_cube !== undefined) {
        miniChartData.timestamps.push(now);
        miniChartData.cube.push(data.t_cube);
        miniChartData.columnTop.push(data.t_column_top || null);
        miniChartData.reflux.push(data.t_reflux || null);

        // Ограничить количество точек
        if (miniChartData.timestamps.length > MINI_CHART_MAX_POINTS) {
            miniChartData.timestamps.shift();
            miniChartData.cube.shift();
            miniChartData.columnTop.shift();
            miniChartData.reflux.shift();
        }

        // Обновить график
        miniChart.updateSeries([
            {
                name: 'Куб',
                data: miniChartData.timestamps.map((t, i) => ({
                    x: t,
                    y: miniChartData.cube[i]
                }))
            },
            {
                name: 'Царга верх',
                data: miniChartData.timestamps.map((t, i) => ({
                    x: t,
                    y: miniChartData.columnTop[i]
                }))
            },
            {
                name: 'Дефлегматор',
                data: miniChartData.timestamps.map((t, i) => ({
                    x: t,
                    y: miniChartData.reflux[i]
                }))
            }
        ]);
    }
}

// ============================================================================
// UI Updates
// ============================================================================

function updateUI(data) {
    // Режим
    if (data.mode !== undefined) {
        const modeNames = ['IDLE', 'RECT', 'MANUAL', 'DIST', 'MASH', 'HOLD'];
        const modeName = modeNames[data.mode] || 'UNKNOWN';
        const modeEl = document.getElementById('mode');
        modeEl.textContent = modeName;
        modeEl.className = `value mode-${modeName.toLowerCase()}`;
    }

    // Фаза
    if (data.phase !== undefined) {
        const phaseNames = ['IDLE', 'HEATING', 'STABIL', 'HEADS', 'PURGE', 'BODY', 'TAILS', 'FINISH', 'ERROR'];
        document.getElementById('phase').textContent = phaseNames[data.phase] || '—';
    }

    // Температуры
    if (data.t_cube !== undefined) {
        document.getElementById('temp-cube').textContent = data.t_cube.toFixed(1) + '°C';
    }
    if (data.t_column_bottom !== undefined) {
        document.getElementById('temp-column-bottom').textContent = data.t_column_bottom.toFixed(1) + '°C';
    }
    if (data.t_column_top !== undefined) {
        document.getElementById('temp-column-top').textContent = data.t_column_top.toFixed(1) + '°C';
    }
    if (data.t_reflux !== undefined) {
        document.getElementById('temp-reflux').textContent = data.t_reflux.toFixed(1) + '°C';
    }
    if (data.t_tsa !== undefined) {
        document.getElementById('temp-tsa').textContent = data.t_tsa.toFixed(1) + '°C';
    }

    // Давление
    if (data.p_cube !== undefined) {
        document.getElementById('pressure-cube').textContent = data.p_cube.toFixed(1) + ' мм рт.ст.';
    }
    if (data.p_atm !== undefined) {
        document.getElementById('pressure-atm').textContent = data.p_atm.toFixed(1) + ' гПа';
    }
    if (data.p_flood !== undefined) {
        document.getElementById('pressure-flood').textContent = data.p_flood.toFixed(1) + ' мм';
    }

    // Мощность (PZEM-004T)
    if (data.voltage !== undefined) {
        document.getElementById('power-voltage').textContent = data.voltage.toFixed(1) + ' V';
    }
    if (data.current !== undefined) {
        document.getElementById('power-current').textContent = data.current.toFixed(2) + ' A';
    }
    if (data.power !== undefined) {
        document.getElementById('power-power').textContent = data.power.toFixed(0) + ' W';
    }
    if (data.energy !== undefined) {
        document.getElementById('power-energy').textContent = data.energy.toFixed(3) + ' кВт·ч';
    }
    if (data.frequency !== undefined) {
        document.getElementById('power-frequency').textContent = data.frequency.toFixed(1) + ' Гц';
    }
    if (data.pf !== undefined) {
        document.getElementById('power-pf').textContent = data.pf.toFixed(2);
    }

    // Насос
    if (data.pump_speed !== undefined) {
        document.getElementById('pump-speed').textContent = data.pump_speed.toFixed(0) + ' мл/ч';
    }
    if (data.pump_volume !== undefined) {
        document.getElementById('pump-volume').textContent = data.pump_volume.toFixed(0) + ' мл';
    }

    // Объёмы фракций
    if (data.volume_heads !== undefined) {
        document.getElementById('volume-heads').textContent = data.volume_heads.toFixed(0) + ' мл';
    }
    if (data.volume_body !== undefined) {
        document.getElementById('volume-body').textContent = data.volume_body.toFixed(0) + ' мл';
    }
    if (data.volume_tails !== undefined) {
        document.getElementById('volume-tails').textContent = data.volume_tails.toFixed(0) + ' мл';
    }

    // Ареометр
    if (data.abv !== undefined) {
        document.getElementById('abv').textContent = data.abv.toFixed(1) + '%';
    }

    // Uptime
    if (data.uptime !== undefined) {
        document.getElementById('uptime').textContent = formatUptime(data.uptime);
    }

    // События
    if (data.type === 'event') {
        addLog(data.message, data.level || 'info');
    }

    // Обновить мини-график
    updateMiniChart(data);
}

function formatUptime(seconds) {
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = seconds % 60;
    return `${pad(hours)}:${pad(minutes)}:${pad(secs)}`;
}

function pad(num) {
    return num.toString().padStart(2, '0');
}

// ============================================================================
// Tabs
// ============================================================================

function initTabs() {
    const tabs = document.querySelectorAll('.tab');
    tabs.forEach(tab => {
        tab.addEventListener('click', () => {
            const targetId = tab.getAttribute('data-tab');

            // Убрать активный класс со всех вкладок
            tabs.forEach(t => t.classList.remove('active'));
            document.querySelectorAll('.tab-content').forEach(content => {
                content.classList.remove('active');
            });

            // Добавить активный класс к выбранной вкладке
            tab.classList.add('active');
            document.getElementById(targetId).classList.add('active');
        });
    });
}

// ============================================================================
// Control Functions
// ============================================================================

function startRectification() {
    sendCommand('start', 'rectification');
    addLog('▶️ Запуск авто-ректификации', 'info');
}

function startManual() {
    sendCommand('start', 'manual');
    addLog('▶️ Запуск ручного режима', 'info');
}

function startDistillation() {
    sendCommand('start', 'distillation');
    addLog('▶️ Запуск дистилляции', 'info');
}

function stopProcess() {
    if (confirm('Остановить процесс?')) {
        sendCommand('stop');
        addLog('⏹️ Остановка процесса', 'warning');
    }
}

function pauseProcess() {
    sendCommand('pause');
    addLog('⏸️ Пауза', 'info');
}

function resumeProcess() {
    sendCommand('resume');
    addLog('⏯️ Продолжение', 'info');
}

function updateHeater(value) {
    document.getElementById('heater-value').textContent = value;
    sendCommand('heater', 'power', parseInt(value));
}

function updatePump(value) {
    document.getElementById('pump-value').textContent = value;
    sendCommand('pump', 'speed', parseInt(value));
}

function toggleValve(name) {
    sendCommand('valve', name, 1);
    addLog(`🔄 Переключение клапана: ${name}`);
}

// ============================================================================
// Settings
// ============================================================================

function saveWiFi() {
    const ssid = document.getElementById('wifi-ssid').value;
    const password = document.getElementById('wifi-password').value;

    if (ssid) {
        sendCommand('wifi', 'save', 0);
        addLog('💾 WiFi настройки сохранены', 'info');
        alert('WiFi настройки сохранены. Перезагрузите контроллер.');
    }
}

function saveEquipment() {
    const heaterPower = document.getElementById('heater-power-w').value;
    const columnHeight = document.getElementById('column-height').value;

    sendCommand('equipment', 'save', 0);
    addLog('💾 Настройки оборудования сохранены', 'info');
}

function setTheme(theme) {
    document.body.setAttribute('data-theme', theme);
    localStorage.setItem('theme', theme);

    // Обновить тему мини-графика
    if (miniChart) {
        miniChart.updateOptions({
            theme: {
                mode: theme
            }
        });
    }

    addLog(`🎨 Тема изменена: ${theme}`, 'info');
}

function loadTheme() {
    const savedTheme = localStorage.getItem('theme') || 'light';
    document.body.setAttribute('data-theme', savedTheme);
}

// ============================================================================
// Logs
// ============================================================================

function addLog(message, type = 'info') {
    const logContainer = document.getElementById('log-container');
    const timestamp = new Date().toLocaleTimeString();
    const entry = document.createElement('div');
    entry.className = `log-entry ${type}`;
    entry.textContent = `[${timestamp}] ${message}`;

    logContainer.appendChild(entry);

    // Автоскролл вниз
    logContainer.scrollTop = logContainer.scrollHeight;

    // Ограничить количество записей (последние 100)
    const entries = logContainer.querySelectorAll('.log-entry');
    if (entries.length > 100) {
        entries[0].remove();
    }
}

function clearLogs() {
    if (confirm('Очистить логи?')) {
        document.getElementById('log-container').innerHTML = '';
        addLog('Логи очищены', 'info');
    }
}

function downloadLogs() {
    addLog('📥 Запрос экспорта логов...', 'info');
    window.open('/api/export', '_blank');
}
