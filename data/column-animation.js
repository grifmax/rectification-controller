// Animated Column Scheme Controller

class ColumnAnimation {
    constructor() {
        this.cube = document.querySelector('.cube');
        this.powerBtn = document.querySelector('.power-btn');
        this.tap = document.querySelector('.tap');
        this.fractionBadge = document.querySelector('.fraction-badge');
        this.fractionNumber = document.querySelector('.fraction-number');
        this.pumpWheel = document.querySelector('.pump-wheel');
        this.flowPipes = document.querySelectorAll('.flow-pipe');

        // Банки
        this.jars = {
            heads: {
                liquid: document.getElementById('liquid-heads'),
                volume: document.getElementById('vol-heads')
            },
            body: {
                liquid: document.getElementById('liquid-body'),
                volume: document.getElementById('vol-body')
            },
            tails: {
                liquid: document.getElementById('liquid-tails'),
                volume: document.getElementById('vol-tails')
            }
        };

        // Температуры
        this.tempLabels = {
            cube: document.getElementById('temp-cube'),
            column: document.getElementById('temp-column'),
            reflux: document.getElementById('temp-reflux')
        };

        this.currentPhase = 'IDLE';
        this.currentFraction = 0; // 0=none, 1=heads, 2=body, 3=tails
    }

    // Обновить состояние на основе данных
    updateState(data) {
        // Режим и фаза
        if (data.mode !== undefined) {
            this.updateMode(data.mode);
        }

        if (data.phase !== undefined) {
            this.updatePhase(data.phase);
        }

        // Температуры
        if (data.t_cube !== undefined && this.tempLabels.cube) {
            this.tempLabels.cube.textContent = data.t_cube.toFixed(1) + '°C';
        }
        if (data.t_column_top !== undefined && this.tempLabels.column) {
            this.tempLabels.column.textContent = data.t_column_top.toFixed(1) + '°C';
        }
        if (data.t_reflux !== undefined && this.tempLabels.reflux) {
            this.tempLabels.reflux.textContent = data.t_reflux.toFixed(1) + '°C';
        }

        // Объёмы фракций
        if (data.volume_heads !== undefined) {
            this.updateJar('heads', data.volume_heads);
        }
        if (data.volume_body !== undefined) {
            this.updateJar('body', data.volume_body);
        }
        if (data.volume_tails !== undefined) {
            this.updateJar('tails', data.volume_tails);
        }

        // Скорость насоса
        if (data.pump_speed !== undefined) {
            this.updatePump(data.pump_speed);
        }
    }

    // Обновить режим
    updateMode(mode) {
        // 0=IDLE, 1=RECT, 2=MANUAL, 3=DIST, 4=MASH, 5=HOLD
        if (mode === 0) { // IDLE
            this.stopAll();
        } else {
            this.startPower();
        }
    }

    // Обновить фазу
    updatePhase(phase) {
        // 0=IDLE, 1=HEATING, 2=STABIL, 3=HEADS, 4=PURGE, 5=BODY, 6=TAILS, 7=FINISH, 8=ERROR
        this.currentPhase = phase;

        switch(phase) {
            case 0: // IDLE
                this.stopAll();
                break;
            case 1: // HEATING
                this.startHeating();
                this.stopCooling();
                this.stopTap();
                this.currentFraction = 0;
                break;
            case 2: // STABILIZATION
                this.startHeating();
                this.startCooling();
                this.stopTap();
                this.currentFraction = 0;
                break;
            case 3: // HEADS
                this.startHeating();
                this.startCooling();
                this.startTap();
                this.currentFraction = 1;
                this.updateFractionIndicator(1);
                break;
            case 4: // PURGE
                this.startHeating();
                this.startCooling();
                this.stopTap();
                this.currentFraction = 0;
                break;
            case 5: // BODY
                this.startHeating();
                this.startCooling();
                this.startTap();
                this.currentFraction = 2;
                this.updateFractionIndicator(2);
                break;
            case 6: // TAILS
                this.startHeating();
                this.startCooling();
                this.startTap();
                this.currentFraction = 3;
                this.updateFractionIndicator(3);
                break;
            case 7: // FINISH
                this.stopAll();
                break;
            case 8: // ERROR
                this.stopAll();
                break;
        }
    }

    // Запустить нагрев куба
    startHeating() {
        if (this.cube) {
            this.cube.classList.add('heating');
        }
    }

    stopHeating() {
        if (this.cube) {
            this.cube.classList.remove('heating');
        }
    }

    // Запустить охлаждение
    startCooling() {
        // Анимация уже есть в CSS для cooling-ring
        // Можно добавить дополнительный класс если нужно
    }

    stopCooling() {
        // Можно убрать класс если добавляли
    }

    // Запустить кнопку Power
    startPower() {
        if (this.powerBtn) {
            this.powerBtn.classList.add('active');
        }
    }

    stopPower() {
        if (this.powerBtn) {
            this.powerBtn.classList.remove('active');
        }
    }

    // Запустить кран (капли)
    startTap() {
        if (this.tap) {
            this.tap.classList.add('dripping');
        }
        if (this.fractionBadge) {
            this.fractionBadge.classList.add('active');
        }
        // Запустить анимацию труб
        this.flowPipes.forEach(pipe => pipe.classList.add('flowing'));
    }

    stopTap() {
        if (this.tap) {
            this.tap.classList.remove('dripping');
        }
        if (this.fractionBadge) {
            this.fractionBadge.classList.remove('active');
        }
        // Остановить анимацию труб
        this.flowPipes.forEach(pipe => pipe.classList.remove('flowing'));
    }

    // Обновить индикатор фракции
    updateFractionIndicator(fraction) {
        if (this.fractionNumber) {
            this.fractionNumber.textContent = fraction;
        }
    }

    // Обновить насос
    updatePump(speed) {
        if (this.pumpWheel) {
            if (speed > 0) {
                this.pumpWheel.classList.add('running');
                // Можно менять скорость анимации в зависимости от speed
                const animationSpeed = Math.max(0.5, 3 - (speed / 500)); // быстрее при большей скорости
                this.pumpWheel.style.animationDuration = animationSpeed + 's';
            } else {
                this.pumpWheel.classList.remove('running');
            }
        }
    }

    // Обновить банку
    updateJar(jarType, volume) {
        const jar = this.jars[jarType];
        if (!jar) return;

        const maxVolume = 2000; // мл, максимальный объём банки
        const maxHeight = 100; // высота области для жидкости в SVG

        // Рассчитать высоту жидкости
        const liquidHeight = Math.min((volume / maxVolume) * maxHeight, maxHeight);

        // Обновить высоту жидкости (рисуем снизу вверх)
        if (jar.liquid) {
            jar.liquid.setAttribute('height', liquidHeight);
            jar.liquid.setAttribute('y', maxHeight + 20 - liquidHeight);

            // Добавить эффект наполнения если растёт
            jar.liquid.classList.add('jar-filling');
            setTimeout(() => {
                jar.liquid.classList.remove('jar-filling');
            }, 500);
        }

        // Обновить текст объёма
        if (jar.volume) {
            jar.volume.textContent = Math.round(volume) + 'ml';
        }
    }

    // Остановить всё
    stopAll() {
        this.stopHeating();
        this.stopCooling();
        this.stopPower();
        this.stopTap();
        if (this.pumpWheel) {
            this.pumpWheel.classList.remove('running');
        }
        this.currentFraction = 0;
    }
}

// Глобальный экземпляр
let columnAnim = null;

// Инициализация при загрузке DOM
document.addEventListener('DOMContentLoaded', function() {
    // Проверить наличие SVG на странице
    const svg = document.getElementById('column-scheme');
    if (svg) {
        columnAnim = new ColumnAnimation();
        console.log('Column animation initialized');
    }
});

// Функция для обновления из главного app.js
function updateColumnAnimation(data) {
    if (columnAnim) {
        columnAnim.updateState(data);
    }
}
